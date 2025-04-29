
#include "Component.hpp"

// Including used XType classes
#include "Interface.hpp"
#include "InterfaceModel.hpp"
#include "ComponentModel.hpp"

using namespace xtypes;

// Constructor
xtypes::Component::Component(const std::string& classname) : _Component(classname)
{
    // NOTE: Properties and relations have been created in _Component constructor
    // Here, we register ourselves and the xtypes we use to our registry
}

// Static identifier
const std::string xtypes::Component::classname = "xtypes::Component";

// Method implementations
// Returns the model of which this component has been instantiated from
ComponentModelPtr xtypes::Component::get_type()
{
    return (std::static_pointer_cast<ComponentModel>(this->get_facts("model")[0].target.lock()));
}

// This function creates an instance of the ComponentModel
void xtypes::Component::instance_of(ComponentModelCPtr model)
{
    this->add_model(model);
}

// This function adds a part to the ComponentModel
void xtypes::Component::part_of(ComponentModelCPtr whole)
{
    this->add_whole(whole);
}

// This function adds an interface to the ComponentModel
void xtypes::Component::has(InterfaceCPtr interface)
{
    this->add_interfaces(interface);
}

// This function returns interface or a nullptr
InterfaceCPtr xtypes::Component::get_interface(const std::string &name)
{
    if (this->has_facts("interfaces"))
    {
        const std::vector<xtypes::Fact> &interfaces(this->get_facts("interfaces"));
        for (const auto &[i, _] : interfaces)
        {
            const auto &interf(i.lock());
            if (interf->get_property("name") == name)
                return std::static_pointer_cast<Interface>(interf);
        }
    }
    return nullptr;
}

// Try to match every interface to a model interface by name and type. If such a match cannot be made the interface is added to the list returned together with type matching interfaces.
std::map<InterfacePtr, std::vector<InterfacePtr>> xtypes::Component::find_nonmatching_interfaces()
{
    std::map<InterfacePtr, std::vector<InterfacePtr>> result;
    if (!this->has_facts("interfaces"))
    {
        return result;
    }
    const xtypes::ComponentModelPtr model(std::static_pointer_cast<ComponentModel>(this->get_facts("model")[0].target.lock()));
    // First pass: find exact matches and prefill nonmatching map
    // NOTE: The exact_matches set tracks the matched MODEL interface uri
    std::set<std::string> exact_matches;
    // NOTE: The no_matches set tracks the not matching INSTANCE interfaces
    std::set<InterfacePtr> no_matches;
    for (const auto& [i,_] : this->get_facts("interfaces"))
    {
        const xtypes::InterfacePtr &interface(std::static_pointer_cast<Interface>(i.lock()));
        // Find an exact match
        const std::vector<InterfacePtr> matches(model->get_interfaces(interface->get_type(), interface->get_name()));
        if (matches.size() == 1)
        {
            exact_matches.insert(matches[0]->uri());
        } else if (matches.size() > 1)
        {
            throw std::runtime_error("xtypes::Component::find_nonmatching_interfaces(): Found multiple matches by type and name!!!");
        } else {
            no_matches.insert(interface);
        }
    }
    // Second pass: find possible matches (without the ones which already have an exact match)
    for (const auto& interface : no_matches)
    {
        // Find compatible interfaces
        std::vector<InterfacePtr> compatible(model->get_interfaces(interface->get_type()));
        // Filter out those interfaces which are in the exact_matches set
        // NOTE: Erase-remove idiom
        compatible.erase(std::remove_if(compatible.begin(), compatible.end(),
                    [&exact_matches](InterfacePtr i)
                    {
                        return (exact_matches.count(i->uri()) > 0);
                    }
                    ), compatible.end());
        result[interface] = compatible;
    }
    return result;
}

// Returns the alias if given and not-empty. Otherwise returns the name.
std::string xtypes::Component::alias_or_name()
{
    const std::string& alias(this->get_alias());
    return alias.empty() ? this->get_name() : alias;
}

// Overrides for setters of properties

// Overrides for relation setters


void xtypes::Component::add_model(xtypes::ComponentModelCPtr xtype, const nl::json& props)
{
    // Add your advanced code here
    if (this->has_facts("whole"))
    {
        const auto &the_facts(this->get_facts("whole"));
        if ((the_facts.size() > 0) && the_facts[0].target.lock()->uuid() == xtype->uuid())
            throw std::invalid_argument("xtypes::Component::add_model: Cannot be an instance of my whole");
    }
    // Finally call the overridden method
    this->_Component::add_model(xtype, props);
}

void xtypes::Component::add_whole(xtypes::ComponentModelCPtr xtype, const nl::json& props)
{
    // Add your advanced code here
    if (this->has_facts("model"))
    {
        const auto &the_facts(this->get_facts("model"));
        if ((the_facts.size() > 0) && the_facts[0].target.lock()->uuid() == xtype->uuid())
            throw std::invalid_argument("xtypes::Component::add_whole(): Cannot be part of my own model");
    }
    // Finally call the overridden method
    this->_Component::add_whole(xtype, props);
}

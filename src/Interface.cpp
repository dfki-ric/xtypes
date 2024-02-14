#include "Interface.hpp"

// Including used XType classes
#include "Component.hpp"
#include "ComponentModel.hpp"
#include "DynamicInterface.hpp"
#include "InterfaceModel.hpp"
#include "Module.hpp"
#include <iostream>

using namespace xtypes;

// Constructor
xtypes::Interface::Interface(const std::string &classname) : _Interface(classname)
{
    // NOTE: Properties and relations have been created in _Interface constructor
}

// Static identifier
const std::string xtypes::Interface::classname = "xtypes::Interface";

// Method implementations
// This function returns the domain of the interface
std::string xtypes::Interface::get_domain()
{
    return this->get_type()->get_domain();
}

// Returns the model of which this interface has been instantiated from
InterfaceModelPtr xtypes::Interface::get_type()
{
    return (std::static_pointer_cast<InterfaceModel>(this->get_facts("model")[0].target.lock()));
}

// TThis function adds a instance to the InterfaceModel
void xtypes::Interface::instance_of(const InterfaceModelPtr model)
{
    this->add_model(model);
}

// This function makes an interface a child of a parent interface model, a component model or a component instance
void xtypes::Interface::child_of(const xtypes::XTypePtr parent)
{
    if (this->isinstance<Component>(parent.get()))
    {
        this->add_parent(std::static_pointer_cast<Component>(parent));
    }
    else if (this->isinstance<ComponentModel>(parent.get()))
    {
        this->add_parent(std::static_pointer_cast<ComponentModel>(parent));
    }
    else
    {
        throw std::invalid_argument("Interface.child_of(): parent xtype " + parent->get_classname() + " is not allowed");
    }
}

// This function makes an interface an alias of another interface.
// NOTE: This is used to e.g. bind an internal interface of a component model to an external one.
void xtypes::Interface::alias_of(const InterfacePtr interface)
{
    this->add_original(interface);
}

// Returns true if this interface is connected to the other interface
bool xtypes::Interface::is_connected_to(const InterfacePtr other)
{
    for (const auto& [i,_] : get_facts("others"))
    {
        const InterfacePtr interface(std::static_pointer_cast<Interface>(i.lock()));
        if (interface->uri() == other->uri())
            return true;
    }
    return false;
}

// This function first checks if the interfaces can be connected. If so, they get connected.
bool xtypes::Interface::connected_to(const InterfacePtr interface, const nl::json &properties)
{
    // First check if both interfaces are ALREADY connected
    if (is_connected_to(interface))
    {
        // TODO: Should we update the properties?
        return true;
    }
    if (!is_connectable_to(interface))
    {
        // NOTE: Removed throw here, because trying to connect incompatible interfaces is ok and not considered a serious error
        std::cerr << "Interface.connected_to(): Interface " + this->get_name() + "  cannot be connected to  " + interface->get_name() << "\n";
        return false;
    }

    // NOTE: Our setters have been disabled and would throw, so we have to call the base class functions here
    this->_Interface::add_others(interface, properties);
    return true;
}

// This function disconnects this interface from all others
void xtypes::Interface::disconnect()
{
    // Disconnect others from us first
    std::vector<xtypes::Fact> others(this->get_facts("others"));
    for (const auto &[o, _] : others)
    {
        const xtypes::XTypePtr other(o.lock());
        std::vector<XTypePtr> to_remove;
        // Gather info at other interfaces to be deleted
        std::vector<xtypes::Fact> from_others(other->get_facts("from_others"));
        for (const auto &[fo, _] : from_others)
        {
            const xtypes::XTypePtr from_other(fo.lock());
            if (from_other->uuid() == this->uuid())
            {
                other->remove_fact("from_others", from_other);
            }
        }
    }
    this->facts.at("others").clear();
}

// This function check connectability
bool xtypes::Interface::is_connectable_to(const InterfacePtr other)
{
    if (!this->is_compatible_with(other))
    {
        return false;
    }
    if (this->get_multiplicity() == "ONE" && (this->get_facts("others").size() > 0 || this->get_facts("from_others").size() > 0))
    {
        std::cout << "Interface.is_connectable_to(): multiplicity of source interface is violated" << std::endl;
        return false;
    }
    else if (other->get_multiplicity() == "ONE" && (other->get_facts("others").size() > 0 || other->get_facts("from_others").size() > 0))
    {
        std::cout << "Interface.is_connectable_to(): multiplicity of target interface is violated" << std::endl;
        return false;
    }
    else if (this->get_multiplicity() == "MULTIPLICITY_NOT_SET" || other->get_multiplicity() == "MULTIPLICITY_NOT_SET")
    {
        std::cout << "Interface.is_connectable_to(): multiplicity unspecified" << std::endl;
        return true;
    }
    return true;
}

// This function checks if this and the other interface are compatible (type, direction, etc).
bool xtypes::Interface::is_compatible_with(const InterfacePtr other)
{
    if (!this->has_same_type(other))
    {
        std::cout << "Interface.is_compatible_with(): Type mismatch" << std::endl;
        return false;
    }
    if ((this->get_direction() == "OUTGOING" && other->get_direction() == "INCOMING") ||
        (this->get_direction() == "INCOMING" && other->get_direction() == "OUTGOING")
    )
        return true;
    else if (this->get_direction() == "BIDIRECTIONAL" && other->get_direction() == "BIDIRECTIONAL")
        return true;
    else if (this->get_direction() == "DIRECTION_NOT_SET" && other->get_direction() == "DIRECTION_NOT_SET")
        return true;
    std::cout << "Interface.is_compatible_with(): Direction mismatch" << std::endl;
    return false;
}

// This function returns true if both interfaces share the same type/InterfaceModel
bool xtypes::Interface::has_same_type(const InterfacePtr other)
{
    return (this->get_type()->uuid() == other->get_type()->uuid());
}

// This function returns true if this interface can realize another
bool xtypes::Interface::can_realize(const InterfacePtr other)
{
    // Realizing another interface means, that
    // a) the model of this interface IMPLEMENTS the model of the other
    // NOTE: Because we do not yet have abstract interface models, we just have to check if the models are the same
    if (!this->has_same_type(other))
        return false;
    // b) the directions are equal
    if (this->get_direction() != other->get_direction())
        return false;
    return true;
}

// This returns the  DynamicInterface instance related to this resolved interface if it has one, otherwise nullptr
DynamicInterfacePtr xtypes::Interface::get_dynamic_interface()
{
    // An interface has to have a parent component
    const xtypes::ComponentPtr parent(std::static_pointer_cast<Component>(this->get_facts("parent")[0].target.lock()));
    // and that has a component model
    const xtypes::ComponentModelPtr parentModel(parent->get_type());
    // and dynamic interfaces
    for (const auto &[di, _] : parentModel->get_facts("dynamic_interfaces"))
    {
        const DynamicInterfacePtr dynIf(std::static_pointer_cast<DynamicInterface>(di.lock()));
        // which matches type
        if (dynIf->get_type()->uuid() != this->get_type()->uuid())
            continue;
        // and direction
        if (dynIf->get_direction() != this->get_direction())
            continue;
        // matches, so we return it
        return dynIf;
    }
    // no matches found
    return nullptr;
}

// This function states that this Interface is the realization of an Interface of an Abstract ComponentModel
void xtypes::Interface::realizes(const InterfacePtr interface_of_abstract_component_model)
{
    this->add_interfaces_of_abstracts(interface_of_abstract_component_model);
}

// Overrides for setters of properties

// Overrides for relation setters

void xtypes::Interface::add_others(xtypes::InterfaceCPtr xtype, const nl::json &props)
{
    // Add your advanced code here
    throw std::invalid_argument("xtypes::Interface::add_others(): Not allowed. Use connected_to() instead.");
    // Finally call the overridden method
    this->_Interface::add_others(xtype, props);
}

void xtypes::Interface::add_from_others(xtypes::InterfaceCPtr xtype, const nl::json &props)
{
    // Add your advanced code here
    throw std::invalid_argument("xtypes::Interface::add_from_others(): Not allowed. Use connected_to() instead.");
    // Finally call the overridden method
    this->_Interface::add_from_others(xtype, props);
}

void xtypes::Interface::add_parent(xtypes::ComponentCPtr xtype, const nl::json& props)
{
    // Add your advanced code here
    bool has_parent = (this->has_facts("parent") && (this->get_facts("parent").size() > 0));
    if (has_parent)
    {
        throw std::invalid_argument("xtypes::Interface::add_parent(): Already has a parent");
    }
    // Finally call the overridden method
    this->_Interface::add_parent(xtype, props);
}

void xtypes::Interface::add_parent(xtypes::ComponentModelCPtr xtype, const nl::json& props)
{
    // Add your advanced code here
    bool has_parent = (this->has_facts("parent") && (this->get_facts("parent").size() > 0));
    if (has_parent)
    {
        throw std::invalid_argument("xtypes::Interface::add_parent(): Already has a parent");
    }
    // Finally call the overridden method
    this->_Interface::add_parent(xtype, props);
}

void xtypes::Interface::add_parent(xtypes::ModuleCPtr xtype, const nl::json& props)
{
    // Add your advanced code here
    bool has_parent = (this->has_facts("parent") && (this->get_facts("parent").size() > 0));
    if (has_parent)
    {
        throw std::invalid_argument("xtypes::Interface::add_parent(): Already has a parent");
    }
    // Finally call the overridden method
    this->_Interface::add_parent(xtype, props);
}

void xtypes::Interface::add_interfaces_of_abstracts(xtypes::InterfaceCPtr xtype, const nl::json &props)
{
    // Checking as alias
    if (!this->can_realize(xtype))
    {
        throw std::invalid_argument("xtypes::Interface::add_interfaces_of_abstracts(): This Interface can not realize the other");
    }
    // Finally call the overridden method
    this->_Interface::add_interfaces_of_abstracts(xtype, props);
}

// This function removes the realization which has been done with the abstract interface
void xtypes::Interface::unrealize()
{
    if (this->has_facts("interfaces_of_abstracts"))
    {
        this->facts.at("interfaces_of_abstracts").clear();
    }
}

// This function returns true if this interface has already realized an abstract interface
bool xtypes::Interface::has_realization(const InterfacePtr other)
{
    for (const auto &[a_inter, _] : this->get_facts("interfaces_of_abstracts"))
    {
        const InterfacePtr inter(std::static_pointer_cast<Interface>(a_inter.lock()));
        if (inter->uuid() == other->uuid())
            return true;
    }
    return false;
}

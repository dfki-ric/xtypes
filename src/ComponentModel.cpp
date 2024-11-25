#include <stdexcept>
#include <fstream>
#include <iostream>
#include "ComponentModel.hpp"

// Including used XType classes
#include "Component.hpp"
#include "DynamicInterface.hpp"
#include "Interface.hpp"
#include "Module.hpp"
#include "InterfaceModel.hpp"
#include "ExternalReference.hpp"
#include "AutoprojReference.hpp"
#include <xtypes_generator/utils.hpp>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#endif
using namespace xtypes;

#include <deque>

// Constructor
xtypes::ComponentModel::ComponentModel(const std::string &classname) : _ComponentModel(classname)
{
    // NOTE: Properties and relations have been created in _ComponentModel constructor
}

// Static identifier
const std::string xtypes::ComponentModel::classname = "xtypes::ComponentModel";

// Method implementations
// This function derives the domain from it's parts. Mixing parts of different domains will make it an ASSEMBLY
bool xtypes::ComponentModel::derive_domain_from_parts()
{
    // TODO: We could call this function everytime we ADD or REMOVE a part :D
    std::string derived_domain;
    for (const auto &p : this->get_facts("parts"))
    {
        const xtypes::ComponentPtr part(std::static_pointer_cast<Component>(p.target.lock()));
        if (derived_domain.empty())
        {
            // Set the initial domain to the domain of the first part
            derived_domain = part->get_type()->get_property("domain");
        }
        else if (derived_domain != part->get_type()->get_property("domain"))
        {
            // If one of the other parts has a different domain, we get an assembly component
            derived_domain = "ASSEMBLY";
            break;
        }
    }
    // Update the domain if it could have been derived
    if (!derived_domain.empty())
    {
        this->set_property("domain", derived_domain);
        return true;
    }
    return false;
}

// This function returns a part of an ComponentModel with a given name
ComponentPtr xtypes::ComponentModel::get_part(const std::string &name)
{
    const std::vector<xtypes::Fact> &parts(this->get_facts("parts"));
    for (const auto &it : parts)
    {
        const xtypes::XTypePtr part(it.target.lock());
        if (part->get_property("name") == name)
            return std::static_pointer_cast<Component>(part);
    }
    return nullptr;
}

// Returns true if this ComponentModel hasn't any parts
bool xtypes::ComponentModel::is_atomic(const bool& throw_on_inconsistency)
{
    bool has_parts = this->get_facts("parts").size() > 0;
    if (has_parts && !this->get_can_have_parts() && throw_on_inconsistency)
        throw std::runtime_error("xtypes::ComponentModel::is_atomic: ComponentModel, that is defined to not have parts, has parts");
    return !has_parts;
}

// This function adds a part to the ComponentModel
void xtypes::ComponentModel::composed_of(const ComponentPtr part)
{
    this->add_parts(part);
}

// Returns all the direct superclasses of this model
std::vector<ComponentModelPtr> xtypes::ComponentModel::get_types()
{
    std::vector<ComponentModelPtr> result;
    for (const auto& [cm, _] : this->get_facts("model"))
    {
        const xtypes::ComponentModelPtr model(std::static_pointer_cast<ComponentModel>(cm.lock()));
        result.push_back(model);
    }
    return result;
}

// Returns all the superclasses of this model (transitive closure over superclass_of relation)
std::map<std::string, ComponentModelPtr> xtypes::ComponentModel::get_all_types()
{
    std::map<std::string, ComponentModelPtr> result;
    for (const auto& model : this->get_types())
    {
        result[model->uri()] = model;
        result.merge(model->get_all_types());
    }
    return result;
}

// This function sets the superclass of the ComponentModel and updates the type property accordingly
void xtypes::ComponentModel::subclass_of(const ComponentModelPtr superclass)
{
    this->add_model(superclass);
}
//// This function checks whether this ComponentModel is a valid implementation of the abstract ComponentModel superclass
bool xtypes::ComponentModel::is_valid_implementation(const ComponentModelPtr for_superclass) {
    // Ensure we are implementing an Abstract component model
    if(!for_superclass->is_abstract())
    {
        throw std::runtime_error("xtypes::ComponentModel::is_valid_implementation(): superclass \""+for_superclass->get_name()+"\" is not an abstract component model");
    }

    if (this->is_abstract())
    {
        // REVIEW: Maybe we can later on add support for partial implementation of a superclass
        throw std::runtime_error("xtypes::ComponentModel::is_valid_implementation(): An abstract class can't be used to implement an abstract superclass");
    }

    // Check if this model has already implemented the same supermodel before, to avoid duplicate implementations
    for (ComponentModelPtr superclass : this->get_abstracts())
    {
        if (superclass->uri() == for_superclass->uri())
        {
            throw std::runtime_error("xtypes::ComponentModel::is_valid_implementation(): ComponentModel \"" + this->get_name() + "\" has already implemented the abstract model \"" + for_superclass->get_name()+ '\"');
        }
    }
    int matches = 0;
    for (const auto& [ti, _] :  this->get_facts("interfaces"))
    {
        const InterfacePtr this_interface(std::static_pointer_cast<Interface>(ti.lock()));
        for (const auto& [ai, _] :  this_interface->get_facts("interfaces_of_abstracts"))
        {
            const InterfacePtr abstract_interface(std::static_pointer_cast<Interface>(ai.lock()));
            const ComponentModelPtr abstract(std::static_pointer_cast<ComponentModel>(abstract_interface->get_facts("parent").at(0).target.lock()));
            if (abstract->uuid() == for_superclass->uuid()) {
                matches++;
                break;
            }
        }
    }
    return matches == for_superclass->get_facts("interfaces").size();
}

// This function checks whether this ComponentModel can implement superclass abstract ComponentModel.
bool xtypes::ComponentModel::can_implement(const ComponentModelPtr superclass)
{
    // Ensure superclass is an Abstract component model
    if (!superclass->is_abstract())
        return false;

    if (this->is_abstract())
        // TODO: Maybe we can later on add support for partial implementation of a superclass
        return false;
    
    if (superclass->get_facts("interfaces").size() == 0 || this->get_facts("interfaces").size() == 0 )
        return false;
    std::set<std::string> matchedInterfaces;

    // Loop through each interface in B (this)
    for (const auto &[ti, _] : this->get_facts("interfaces"))
    {
        const InterfacePtr this_interface(std::static_pointer_cast<Interface>(ti.lock()));
        // Loop through each interface in A (superclass)
        for (const auto &[ti, _] : superclass->get_facts("interfaces"))
        {
            const InterfacePtr abstract_interface(std::static_pointer_cast<Interface>(ti.lock()));

            if (this_interface->can_realize(abstract_interface))
            {
                // Check if we already tested the realizability between A's interface and B's interface
                if (matchedInterfaces.find(abstract_interface->uri()) != matchedInterfaces.end())
                {
                    // Interface from B is already matched with another interface from A, not a valid match.
                    // We need each interface from A com
                    continue;
                }
                // Match found, print the matched interfaces
                //std::cout << this_interface->get_property("name").get<std::string>() << " can implement "
                //          << abstract_interface->get_property("name").get<std::string>() << std::endl;

                //add the matched interface to the set so we don't check twice
                matchedInterfaces.insert(abstract_interface->uri());
                break; // Move to the next interface in B after finding a match for this_interface
            }
        }
    }
    // Check if the number of matches equals the number of interfaces in A
    // and also check if every interface in A has a unique match in B
    return (matchedInterfaces.size() == superclass->get_facts("interfaces").size()) ;
}

// This function states that this ComponentModel is a valid implementation of the given superclass. Throws if this is not the case. (see is_valid_implementation).
void xtypes::ComponentModel::implements(const ComponentModelPtr superclass)
{
    this->add_abstracts(superclass);
}

// Returns true when this ComponentModel is not yet fully implemented or this is abstract by it's nature (abstract property set to true).
bool xtypes::ComponentModel::is_abstract()
{
    bool is_abstract = this->get_abstract(); 
    if (!is_abstract) 
    {
        for (const auto& [p, _] :  this->get_facts("parts"))
        {
            const ComponentPtr part(std::static_pointer_cast<Component>(p.lock()));
            is_abstract |= part->get_type()->is_abstract();
            if (is_abstract) break;
        }
    }
    return is_abstract;
}

std::vector<ComponentModelPtr> xtypes::ComponentModel::get_abstracts()
{
    std::vector<ComponentModelPtr> abstracts;
    for(const auto& [a, _] :  this->get_facts("abstracts"))
    {
        const ComponentModelPtr abstract(std::static_pointer_cast<ComponentModel>(a.lock()));
        abstracts.push_back(abstract);
    }
    return abstracts;
}

std::vector<ComponentModelPtr> xtypes::ComponentModel::get_implementations()
{
    std::vector<ComponentModelPtr> implementations;
    for(const auto& [i, _] :  this->get_facts("implementations"))
    {
        const ComponentModelPtr impl(std::static_pointer_cast<ComponentModel>(i.lock()));
        implementations.push_back(impl);
    }
    return implementations;
}

// Returns all the abstract models for which this ComponentModel is an implementation (transitive closure over superclass_of relation of abstracts)
std::map<std::string, ComponentModelPtr> xtypes::ComponentModel::get_all_abstracts()
{
    std::map<std::string, ComponentModelPtr> implementations;
    for (const auto& abstract :  this->get_abstracts())
    {
        implementations[abstract->uri()] = abstract;
        implementations.merge(abstract->get_all_abstracts());
    }
    for (const auto& model : this->get_types()) //we are not using get_all_types here as this is resolved by the following recursion anyways
    {
        implementations.merge(model->get_all_abstracts());
    }
    return implementations;
}

// This function checks whether this ComponentModel has the implements or the subclass relation to the given superclass set
bool xtypes::ComponentModel::is_implementing(const ComponentModelPtr superclass)
{
    for (const auto& [uri, abstract] :  this->get_all_abstracts())
    {
        if (superclass->uri() == uri) return true;
    }
    return false;
}

// This function adds an interface to the ComponentModel
void xtypes::ComponentModel::has(const InterfacePtr interface)
{
    // NOTE: Because the URI of the interface depends on the parent itself, we can only use the add_* function of the interface
    // Our URI is valid
    interface->add_parent(std::static_pointer_cast<ComponentModel>(shared_from_this()));
}

// This function adds an dynamic interface to the ComponentModel
void xtypes::ComponentModel::has(const DynamicInterfacePtr dynamic_interface)
{
    // Same as has() for interface
    dynamic_interface->add_parent(std::static_pointer_cast<ComponentModel>(shared_from_this()));
}


// This function creates a new component instance of this model and makes it part of the given component model
// NOTE: Instantiation is only possible if a whole is given of which the new component instance can be a part of.
ComponentPtr xtypes::ComponentModel::instantiate(const ComponentModelPtr as_part_of_whole, const std::string& and_name, const bool& with_empty_facts)
{
    if (!as_part_of_whole)
    {
        // We throw here, because this is a serious usage error
        throw std::invalid_argument("ComponentModel::instantiate: need a whole for which a component is part of");
    }
    XTypeRegistryPtr reg = registry.lock();
    if (!reg)
    {
        throw std::invalid_argument("ComponentModel::instantiate: no registry");
    }

    ComponentPtr comp = reg->instantiate<Component>();
    // set all unknown facts empty (convenience functionality)
    if (with_empty_facts)
    {
        comp->set_all_unknown_facts_empty();
    }
    // set initial properties
    comp->set_properties(this->get_properties(), false);
    // override name if given
    if (!and_name.empty())
    {
        comp->set_name(and_name);
    }
    // update configuration
    comp->set_configuration(this->get_defaultConfiguration());
    // Make the component part of the whole
    as_part_of_whole->composed_of(comp);
    comp->instance_of(std::static_pointer_cast<ComponentModel>(shared_from_this()));
    // inheritance of interfaces
    for (const auto &[i, _] : this->get_facts("interfaces"))
    {
        // for each interface, get the model and instantiate a clone to be used for the new component instance
        const xtypes::InterfacePtr interface(std::static_pointer_cast<Interface>(i.lock()));
        const xtypes::InterfaceModelPtr model(interface->get_type());
        InterfacePtr clone = reg->instantiate<Interface>();
        clone->set_properties(interface->get_properties());
        // set all unknown facts empty (convenience functionality)
        if (with_empty_facts)
        {
            clone->set_all_unknown_facts_empty();
        }
        comp->has(clone);
        clone->instance_of(model);
    }
    // DynamicInterface will be created later
    return comp;
}

// This function builds a new module out of the component model spec. It will also build ALL subcomponents.
ModulePtr xtypes::ComponentModel::build(const std::string& with_name, const std::function< ComponentModelPtr(const ComponentModelPtr&, const std::vector<ComponentModelPtr>&) >& select_implementation)
{
    XTypeRegistryPtr reg = this->registry.lock();
    if (!reg)
    {
        throw std::invalid_argument("ComponentModel::build(): No registry");
    }

    // First check if we are abstract or not
    if (this->get_abstract())
    {
        // We are abstract, so we need to get resolved first
        const std::vector< ComponentModelPtr >& implementations(this->get_implementations());
        if ((implementations.size() > 1) && !select_implementation)
        {
            throw std::invalid_argument("ComponentModel::build: " + this->uri() + " is abstract and has multiple implementations but callback func is missing");
        }
        ComponentModelPtr impl = implementations[0];
        // If we can have multiple implementations, we have to ask for a specific one
        if (implementations.size() > 1)
        {
            impl = select_implementation(std::static_pointer_cast<ComponentModel>(shared_from_this()), implementations);
        }
        // Now we can build a module and be done
        return impl->build(with_name, select_implementation);
    }

    // Setup toplvl module (without parent) first
    ModulePtr module = reg->instantiate<Module>();
    module->set_properties(this->get_properties(), false);
    module->set_name(with_name);
    module->set_configuration(this->get_defaultConfiguration());
    module->set_unknown_fact_empty("whole");
    module->instance_of(std::static_pointer_cast<ComponentModel>(shared_from_this()));

    // Inherit the interfaces of the model!
    for (const auto &[i, _] : this->get_facts("interfaces"))
    {
        // for each interface, get the model and instantiate a clone to be used for the new component instance
        const InterfacePtr interface(std::static_pointer_cast<Interface>(i.lock()));
        const InterfaceModelPtr model(interface->get_type());
        InterfacePtr clone = reg->instantiate<Interface>();
        clone->set_properties(interface->get_properties());
        clone->set_all_unknown_facts_empty();
        module->has(clone);
        clone->instance_of(model);
    }

    // Early exit: No parts
    module->set_unknown_fact_empty("parts");
    if (this->get_facts("parts").size() < 1)
        return module;

    // We use this map to later wire the modules together
    std::map< ModulePtr, ComponentPtr > submodule2part;
    // NOTE: part2submodule would not be injective!!! That means multiple submodules can exist for the same part
    std::map< InterfacePtr, InterfacePtr > implementation2abstract_interface;
    // NOTE: abstract2implementation_interface would not be injective!!! That means multiple implementation interfaces can exist for the same abstract interface
    // Populate initial queue
    std::deque< std::tuple< ComponentPtr, ModulePtr > > to_visit;
    for (const auto &[p, _] : this->get_facts("parts"))
        to_visit.push_back( { std::static_pointer_cast<Component>(p.lock()), module } );
    while (to_visit.size() > 0)
    {
        auto [part, parent_module] = to_visit.front();
        to_visit.pop_front();

        // NOTE: The following code could be part of a Component::build() function
        // Create a submodule from part
        const ComponentModelPtr part_model(part->get_type());
        ComponentModelPtr submodule_model(part_model);
        if (part_model->get_abstract())
        {
            // We have encountered an abstract, which needs to be resolved first
            const std::vector< ComponentModelPtr >& implementations(part_model->get_implementations());
            if ((implementations.size() > 1) && !select_implementation)
            {
                throw std::invalid_argument("ComponentModel::build: " + part_model->uri()
                        + " is abstract, has multiple implementations but callback func is missing. Cannot build submodule for " + part->uri());
            }
            ComponentModelPtr impl = implementations[0];
            if (implementations.size() > 1)
            {
                impl = select_implementation(part_model, implementations);
            }
            // An implementation has been chosen, so we set this as the new submodule_model
            submodule_model = impl;
        }

        ModulePtr submodule = reg->instantiate<Module>();

        // Configuration is tricky:
        // in general the submodule inherits the properties of the part
        submodule->set_properties(part->get_properties());
        // but for the configuration this is not so easy (especially for abstract parts)
        // at first, the component gets the default configuration of its TRUE model
        nl::json submodule_config = submodule_model->get_defaultConfiguration();
        // then that default config gets updated by the part config
        nl::json part_config = part->get_configuration();
        if (part_config.is_object())
        {
            submodule_config.update(part_config, true);
        }
        submodule->set_configuration(submodule_config);

        submodule->instance_of(submodule_model);
        submodule->part_of(parent_module);
        submodule2part[submodule] = part;
        //std::cout << "submodule: " << submodule->get_name() << "\n";

        submodule->set_unknown_fact_empty("interfaces");
        // Inherit the interfaces of the submodule_model (which could be different from the initial abstract part model)
        for (const auto &[i, _] : submodule_model->get_facts("interfaces"))
        {
            // for each interface, get the model and instantiate a clone to be used for the new component instance
            const InterfacePtr interface(std::static_pointer_cast<Interface>(i.lock()));
            const InterfaceModelPtr ifmodel(interface->get_type());
            InterfacePtr clone = reg->instantiate<Interface>();
            clone->set_all_unknown_facts_empty();
            clone->set_properties(interface->get_properties());
            submodule->has(clone);
            clone->instance_of(ifmodel);
        }

        // Resolve injective mapping from implementation to abstract interface
        if (part_model->get_abstract())
        {
            for (const auto &implementation_model_interface : submodule_model->get_interfaces())
            {
                const InterfacePtr implementation_interface(submodule->get_interface(implementation_model_interface->get_name()));
                // Until here, we are safe
                for (const auto &[i,_] : implementation_model_interface->get_facts("interfaces_of_abstracts"))
                {
                    const InterfacePtr abstract_model_interface(std::static_pointer_cast<Interface>(i.lock()));
                    // Here we have to make sure, that the abstract_model is equal to the part_model
                    if (abstract_model_interface->get_facts("parent")[0].target.lock()->uri() != part_model->uri())
                        continue;
                    // Ok, we have found an abstract_model_interface which should be existing as well at the part
                    const InterfacePtr abstract_interface(part->get_interface(abstract_model_interface->get_name()));
                    // This is a safety check to ensure injectivity
                    if (implementation2abstract_interface.count(implementation_interface))
                    {
                        throw std::runtime_error("ComponentModel::build(): implementation2abstract_interface not injective!");
                    }
                    implementation2abstract_interface[implementation_interface] = abstract_interface;
                }
            }
        }

        // Resolve subparts to be transformed to modules
        submodule->set_unknown_fact_empty("parts");
        if (submodule_model->is_atomic())
            continue;
        for (const auto &[p, _] : submodule_model->get_facts("parts"))
            to_visit.push_back( { std::static_pointer_cast<Component>(p.lock()), submodule } );
    }

    // Resolve alias interfaces
    for (const auto &[submodule, part] : submodule2part)
    {
        const ComponentModelPtr whole(std::static_pointer_cast<ComponentModel>(part->get_facts("whole")[0].target.lock()));
        const ModulePtr parent_module(std::static_pointer_cast<Module>(submodule->get_facts("whole")[0].target.lock()));
        // We have two cases here: abstract and non-abstract model of part
        const ComponentModelPtr part_model(part->get_type());
        const ComponentModelPtr submodule_model(submodule->get_type());
        for (const auto &[i,_] : whole->get_facts("interfaces"))
        {
            // Alias interface pair of whole and module counter part
            const InterfacePtr alias_interface(std::static_pointer_cast<Interface>(i.lock()));
            // Resolve original rel. If there is none, we have nothing to do.
            if (alias_interface->get_facts("original").size() < 1)
                continue;
            // Original interface of part
            const InterfacePtr original_interface(std::static_pointer_cast<Interface>(alias_interface->get_facts("original")[0].target.lock()));
            // Check if this interface is pointing to the same part
            const ComponentPtr parent(std::static_pointer_cast<Component>(original_interface->get_facts("parent")[0].target.lock()));
            if (part->uri() != parent->uri())
                continue;
            // The alias interface twin can always be found by name
            InterfacePtr alias_interface_twin(parent_module->get_interface(alias_interface->get_name()));
            if (!alias_interface_twin)
            {
                std::cout << "ComponentModel::build(): Could not resolve interface " << alias_interface->uri() << " of whole " << whole->uri() << " at parent module " << parent_module->uri() << "\n";
                continue;
            }
            // We have found a match of alias and original interface! So now we resolve the counterparts.
            if (part_model->get_abstract())
            {
                // abstract case: we cannot find the counterparts by name, but we have to find the mapping from abstract interface to implementation interface via the models
                // We have to find an implementation interface which is an interface of submodule and is mapped to the original interface
                InterfacePtr original_interface_twin = nullptr;
                for (const auto &[x,_] : submodule->get_facts("interfaces"))
                {
                    const InterfacePtr candidate_if(std::static_pointer_cast<Interface>(x.lock()));
                    if (implementation2abstract_interface.count(candidate_if) < 1)
                        continue;
                    if (implementation2abstract_interface[candidate_if]->uri() == original_interface->uri())
                    {
                        original_interface_twin = candidate_if;
                        break;
                    }
                }
                if (!original_interface_twin)
                {
                    std::cout << "ComponentModel::build(): Could not resolve abstract interface " << original_interface->uri() << " of part " << part->uri() << " at submodule " << submodule->uri() << "\n";
                    continue;
                }
                alias_interface_twin->alias_of(original_interface_twin);
            } else {
                // non-abstract case: for every interface of whole we find an original interface in part, by name we find the corresponding interfaces in parent_module and submodule
                InterfacePtr original_interface_twin(submodule->get_interface(original_interface->get_name()));
                if (!original_interface_twin)
                {
                    std::cout << "ComponentModel::build(): Could not resolve interface " << original_interface->uri() << " of part " << part->uri() << " at submodule " << submodule->uri() << "\n";
                    continue;
                }
                alias_interface_twin->alias_of(original_interface_twin);
            }
        }
    }

    // Wire modules together according to their counterpart connections
    // NOTE: Here we can use the names to look up the module interfaces
    for (const auto &[submodule, part] : submodule2part)
    {
        // Lookup the whole for later ...
        const XTypePtr whole(submodule->get_facts("whole")[0].target.lock());
        for (const auto &[i, _] : part->get_facts("interfaces"))
        {
            const InterfacePtr part_if(std::static_pointer_cast<Interface>(i.lock()));
            InterfacePtr submodule_if;
            // Remap abstract interfaces to the implementation interfaces if needed
            if (part->get_type()->get_abstract())
            {
                for (const auto &[x,_] : submodule->get_facts("interfaces"))
                {
                    const InterfacePtr candidate_if(std::static_pointer_cast<Interface>(x.lock()));
                    if (implementation2abstract_interface.count(candidate_if) < 1)
                        continue;
                    if (implementation2abstract_interface[candidate_if]->uri() == part_if->uri())
                    {
                        submodule_if = candidate_if;
                        break;
                    }
                }
            } else {
                submodule_if = submodule->get_interface(part_if->get_name());
            }
            if (!submodule_if)
            {
                throw std::runtime_error("ComponentModel::build(): Could not map part interface " + part_if->uri() + " to an interface of " + submodule->uri());
            }
            for (const auto &[i2, conn_props] : part_if->get_facts("others"))
            {
                const InterfacePtr other_part_if(std::static_pointer_cast<Interface>(i2.lock()));
                const ComponentPtr other_part(std::static_pointer_cast<Component>(other_part_if->get_facts("parent")[0].target.lock()));
                ModulePtr other_submodule = nullptr;
                for (const auto &[other_submodule_candidate, other_part_candidate] : submodule2part)
                {
                    // The uris of the candidate part and and the part have to match
                    if (other_part_candidate->uri() != other_part->uri())
                        continue;
                    // The whole of the candidate submodule has to match the submodule whole
                    if (other_submodule_candidate->get_facts("whole")[0].target.lock()->uri() != whole->uri())
                        continue;
                    // NOTE: We do not wanna exclude self connections
                    other_submodule = other_submodule_candidate;
                    break;
                }
                if (!other_submodule)
                {
                    throw std::runtime_error("ComponentModel::build(): Could not find submodule for " + other_part->uri());
                }
                InterfacePtr other_submodule_if;
                // Remap abstract interfaces to the implementation interfaces if needed
                if (other_part->get_type()->get_abstract())
                {
                    for (const auto &[x,_] : other_submodule->get_facts("interfaces"))
                    {
                        const InterfacePtr candidate_if(std::static_pointer_cast<Interface>(x.lock()));
                        if (implementation2abstract_interface.count(candidate_if) < 1)
                            continue;
                        if (implementation2abstract_interface[candidate_if]->uri() == other_part_if->uri())
                        {
                            other_submodule_if = candidate_if;
                            break;
                        }
                    }
                } else {
                    other_submodule_if = other_submodule->get_interface(other_part_if->get_name());
                }
                if (!other_submodule_if)
                {
                    throw std::runtime_error("ComponentModel::build(): Could not map part interface " + other_part_if->uri() + " to an interface of " + other_submodule->uri());
                }
                //std::cout << "connection: " << submodule_if->get_name() << " -> " << other_submodule_if->get_name() << "\n";
                submodule_if->connected_to(other_submodule_if, conn_props);
            }
        }
    }

    return module;
}

// Returns all interfaces which match a given type (if set) and a given name (if set).
std::vector<InterfacePtr> xtypes::ComponentModel::get_interfaces(const InterfaceModelPtr with_type, const std::string& with_name)
{
    std::vector<InterfacePtr> matches;
    for (const auto &[i, _] : this->get_facts("interfaces"))
    {
        const xtypes::InterfacePtr interface(std::static_pointer_cast<Interface>(i.lock()));
        if (with_type && with_type->uuid() != interface->get_type()->uuid())
        {
            continue;
        }
        if (!with_name.empty() && with_name != interface->get_name())
        {
            continue;
        }
        matches.push_back(interface);
    }
    return matches;
}

// This function removes interfaces by it's name
void xtypes::ComponentModel::remove_interface(const std::string &name)
{
    auto matches = get_interfaces(nullptr, name);
    for (const auto& match : matches)
        remove_fact("interfaces", match);
}

// This functions resolves any interfaces of inner parts which do not match any of the parts' model interfaces and a list of possible future matches.
std::map<InterfacePtr, std::vector<InterfacePtr>> xtypes::ComponentModel::find_nonmatching_part_interfaces()
{
    std::map<InterfacePtr, std::vector<InterfacePtr>> result;
    for (const auto& [p,_] : this->get_facts("parts"))
    {
        const xtypes::ComponentPtr part(std::static_pointer_cast<Component>(p.lock()));
        const std::map<InterfacePtr, std::vector<InterfacePtr>> nonmatching_part_interfaces(part->find_nonmatching_interfaces());
        result.insert(nonmatching_part_interfaces.begin(), nonmatching_part_interfaces.end());
    }
    return result;
}

// Go to every part and its interfaces and call disconnect
void xtypes::ComponentModel::disconnect_parts()
{
    for (const auto &[part, _] : this->get_facts("parts"))
        for (const auto &[partInterface, _] : part.lock()->get_facts("interfaces"))
            std::static_pointer_cast<Interface>(partInterface.lock())->disconnect();
}

// This method exports a component model to the DROCK BasicModel Json format
std::string xtypes::ComponentModel::export_to_basic_model()
{
    nl::json data;
    data["name"] = get_name();
    data["domain"] = get_domain();
    data["uri"] = uri();
    data["types"] = nl::json::array();
    for (const auto& supermodel : get_types())
    {
        data["types"].push_back({{"name", supermodel->get_property("name")}, {"version", supermodel->get_property("version")}});
    }
    if (get_abstract())
    {
        data["implementations"] = nl::json::array();
        this->set_unknown_fact_empty("implementations");
        for (const auto &model : get_implementations())
        {
            data["implementations"].push_back({{"name", model->get_property("name")}, {"domain", model->get_property("domain")}, {"version", model->get_property("version")}});
        }
    }

    this->set_unknown_fact_empty("abstracts");
    for (const auto &supermodel : get_abstracts())
    {
        data["abstracts"].push_back({{"name", supermodel->get_property("name")}, {"domain", supermodel->get_property("domain")}, {"version", supermodel->get_property("version")}});
    }

    //data["interfaces_of_abstracts"] = nl::json::array();
    for (const auto &[ti, _] : this->get_facts("interfaces"))
    {
        const InterfacePtr this_interface(std::static_pointer_cast<Interface>(ti.lock()));
        this_interface->set_unknown_fact_empty("interfaces_of_abstracts");
        for (const auto &[ai, _] : this_interface->get_facts("interfaces_of_abstracts"))
        {
            const InterfacePtr abstract_interface(std::static_pointer_cast<Interface>(ai.lock()));
            const ComponentModelPtr abstract(std::static_pointer_cast<ComponentModel>(abstract_interface->get_facts("parent").at(0).target.lock()));
            nl::json ioa;
            ioa["abstract_model_name"] = abstract->get_name(); 
            ioa["abstract_interface_direction"] = abstract_interface->get_direction();
            ioa["abstract_interface_name"] = abstract_interface->get_name(); 
            ioa["abstract_interface_type"] = abstract_interface->get_type()->get_property("name");
            data["interfaces_of_abstracts"].push_back(ioa);
        }
    
    }
    if(this->has_facts("configured_for"))
    {
    for (const auto &[h, _] : this->get_facts("configured_for"))
    {
        const xtypes::ComponentModelPtr hardware_model(std::static_pointer_cast<ComponentModel>(h.lock()));

        data["configured_for"]
            .push_back({{"name", hardware_model->get_property("name")},{"version", hardware_model->get_property("version")}, {"uri", hardware_model->uri()}});
    }
    }
    nl::json version;
    version["name"] = get_version();
    version["date"] = get_date();
    version["designedBy"] = get_designedBy();
    version["projectName"] = get_projectName();
    version["maturity"] = get_maturity();
    version["can_have_parts"] = get_can_have_parts();
    version["abstract"] = get_abstract();
    if (const nl::json defaultConfiguration = get_defaultConfiguration(); !defaultConfiguration.empty())
    {
        version["defaultConfiguration"] = defaultConfiguration;
    }

    if (this->has_facts("external_references")) {
        for (const auto &[p, e] : this->get_facts("external_references"))
        {
            const xtypes::ExternalReferencePtr reference(std::static_pointer_cast<ExternalReference>(p.lock()));
            nl::json referenceData = reference->get_properties();
            referenceData["optional"] = e.contains("optional") ? e["optional"].get<bool>() : true;
            version["external_references"].push_back(referenceData);
        }
    }

    if (const nl::json self_data = this->get_data(); !self_data.empty()) 
    {
        version["data"] = self_data;
    }

    for (const auto &[p, _] : this->get_facts("parts"))
    {
        const xtypes::ComponentPtr part(std::static_pointer_cast<Component>(p.lock()));
        nl::json partData;
        partData["name"] = part->get_name();
        partData["alias"] = part->get_alias();
        partData["model"] = nl::json();
        const xtypes::ComponentModelPtr target(part->get_type());
        partData["model"]["name"] = target->get_name();
        partData["model"]["domain"] = target->get_domain();
        partData["model"]["version"] = target->get_version(); // NOTE: added \" to satisfy assertion(type == type) in xtype when importing
        if (part->has_property("configuration") && !part->get_property("configuration").empty())
        {
            nl::json config;
            if (part->get_property("configuration").is_string() && !part->get_property("configuration").get<std::string>().empty())
                config = xtypes::parseJson(part->get_property("configuration").get<std::string>(), "xtypes::ComponentModel::exportToBasicModelJSON()->configuration");
            else
                config = part->get_property("configuration");
            if (config.contains("name"))
            {
                config["name"] = part->get_name();
                version["components"]["configuration"]["nodes"].push_back(config);
            }
        }
        partData["interface_aliases"] = nl::json();
        for (auto &[i, _] : part->get_facts("interfaces"))
        {
            InterfacePtr interface = std::static_pointer_cast<Interface>(i.lock());
            nl::json edgeData;
            // Store the part interface alias
            partData["interface_aliases"][interface->get_name()] = interface->get_alias();
            edgeData["from"]["name"] = part->get_name();
            edgeData["from"]["interface"] = interface->get_name();
            edgeData["from"]["domain"] = interface->get_domain();
            for (auto &[otherInterface, otherInterfaceProps] : interface->get_facts("others"))
            {
                const std::vector<xtypes::Fact> &parent_component = otherInterface.lock()->get_facts("parent");
                const xtypes::ComponentPtr otherPart = std::static_pointer_cast<Component>(parent_component[0].target.lock());
                edgeData["to"]["name"] = otherPart->get_name();
                edgeData["to"]["interface"] = std::static_pointer_cast<Interface>(otherInterface.lock())->get_name();
                edgeData["to"]["domain"] = std::static_pointer_cast<Interface>(otherInterface.lock())->get_domain();

                // Handle edge properties
                const std::vector<std::string> excludes = {"configuration"};
                for (auto &[prop, val] : otherInterfaceProps.items())
                {
                    std::vector<std::string>::const_iterator it = std::find_if(excludes.begin(), excludes.end(), [&prop = prop](const std::string &excluded)
                                                                               { return excluded == prop; });
                    if (it != excludes.end())
                        continue;
                    edgeData[prop] = val;
                }

                // Handle edge configuration
                if (otherInterfaceProps.contains("configuration") && !otherInterfaceProps["configuration"].empty())
                {
                    nl::json config;
                    if (otherInterfaceProps["configuration"].is_string() && !otherInterfaceProps["configuration"].get<std::string>().empty())
                        config = xtypes::parseJson(otherInterfaceProps["configuration"].get<std::string>(), "xtypes::ComponentModel::exportToBasicModelJSON()->configuration2");
                    else
                        config = otherInterfaceProps["configuration"];
                    version["components"]["configuration"]["edges"].push_back(config);
                }
                version["components"]["edges"].push_back(edgeData);
            }
        }
        version["components"]["nodes"].push_back(partData);
    }

    for (const auto &[i, _] : this->get_facts("interfaces"))
    {
        const InterfacePtr interface(std::static_pointer_cast<Interface>(i.lock()));
        nl::json props = interface->get_properties();
        // Special case: domain and type
        props["domain"] = interface->get_domain();
        props["type"] = interface->get_type()->get_property("name");
        if (interface->has_relation("original") && (interface->get_facts("original").size() > 0))
        {
            props["linkToNode"] = std::static_pointer_cast<ComponentModel>(interface->get_facts("original")[0].target.lock()->get_facts("parent")[0].target.lock())->get_name();
            props["linkToInterface"] = std::static_pointer_cast<ComponentModel>(interface->get_facts("original")[0].target.lock())->get_name();
        }
        version["interfaces"].push_back(props);
    }

    for (const auto &[i, _] : this->get_facts("dynamic_interfaces"))
    {
        const DynamicInterfacePtr interface(std::static_pointer_cast<DynamicInterface>(i.lock()));
        nl::json props = interface->get_properties();
        // Special case: domain and type
        props["domain"] = interface->get_domain();
        props["type"] = interface->get_type()->get_property("name");
        /// original is a featured we might add later for dynamic interfaces as well
        version["dynamic_interfaces"].push_back(props);
    }

    data["versions"].push_back(version);
    return data.dump();
}

std::vector<ComponentModelPtr> xtypes::ComponentModel::import_from_basic_model(const std::string &serialized_model, const XTypeRegistryPtr &registry)
{
    std::vector<ComponentModelPtr> result;
    std::map<std::string, ComponentModelPtr> part_models;
    nl::json data = xtypes::parseJson(serialized_model, "xtypes::ComponentModel::import_from_basic_model");
    if (!data.contains("name"))
    {
        throw std::invalid_argument("ComponentModel::import_from_basic_model(): Could not find 'name' in " + data.dump());
    }
    const std::string name(data["name"].get<std::string>());
    if (!data.contains("domain"))
    {
        throw std::invalid_argument("ComponentModel::import_from_basic_model(): Could not find 'domain' in " + data.dump());
    }
    const std::string domain(data["domain"].get<std::string>());

    // Resolve supermodels
    std::vector<ComponentModelPtr> supermodels;
    if (data.contains("types"))
    {
        for (const auto& t : data["types"])
        {
            nl::json props = nl::json{{"name", t["name"]}, {"version", t["version"]}, {"domain", domain}};
            ComponentModel dummy;
            dummy.set_properties(props);
            ComponentModelPtr supermodel = std::static_pointer_cast<ComponentModel>(registry->load_by_uri(dummy.uri()));
            if (!supermodel)
            {
                supermodel = registry->instantiate<ComponentModel>();
                supermodel->set_properties(props);
            }
            supermodels.push_back(supermodel);
        }
    }

    // Resolve configured_for
    std::vector<ComponentModelPtr> configured_for;
    if (data.contains("configured_for"))
    {
        for (const auto &h : data["configured_for"])
        {
            nl::json props = nl::json{{"name", h["name"]}, {"version", h["version"]}, {"uri", h["uri"]}};
            ComponentModelPtr hardware = std::static_pointer_cast<ComponentModel>(registry->load_by_uri(h["uri"]));
            if (!hardware)
            {
                hardware = registry->instantiate<ComponentModel>();
                hardware->set_properties(props);
            }
            configured_for.push_back(hardware);
        }
    }

    // Create model(s)
    if (!data.contains("versions"))
    {
        throw std::invalid_argument("ComponentModel::import_from_basic_model(): Could not find 'versions' in " + data.dump());
    }
    for (const auto &v : data["versions"])
    {
        if (!v.contains("name"))
        {
            std::cerr << "ComponentModel::import_from_basic_model(): Could not find 'name' in versions " << v.dump() << ". Ignoring it.\n";
            continue;
        }
        const std::string version(v["name"].get<std::string>());

        // Setup the properties
        nl::json props;
        // At first, set second lvl properties
        props = v;
        // Then override by toplvl properties
        props.update(data);
        // ... and set version (since this collides with toplvl name)
        props["version"] = version;

        // Create new model with specific version
        ComponentModelPtr model = registry->instantiate<ComponentModel>();
        model->set_all_unknown_facts_empty();
        model->set_properties(props, false);
        // Attach supermodels
        for (auto& supermodel : supermodels)
        {
            // .. to subclass_of relation
            model->subclass_of(supermodel);
        }

        // Attach configured_for
        for (auto &hardware : configured_for)
        {
            // .. to configured_for relation
            model->add_configured_for(hardware);
        }

        // Handle defaultConfigurations
        if (v.contains("defaultConfiguration"))
            model->set_defaultConfiguration(v["defaultConfiguration"]);

        // Resolve/create external references
        if (v.contains("external_references")) {
            assert(v["external_references"].is_array());
            for (const auto &eRefIt : v["external_references"])
            {
                // TODO: Actually, the ExternalReference has to be looked up FIRST (by load_missing_external_reference or such) and only created if not found
                ExternalReferencePtr eRef = registry->instantiate<ExternalReference>();
                eRef->set_properties(eRefIt);
                // Afterwards we annotate the model with the external reference
                model->annotate_with((ExternalReferencePtr)eRef, eRefIt["optional"]);
            }
        }

        // Handle deprecated repository
        if (v.contains("repository"))
            throw std::runtime_error("The 'repository' property of ComponentModel is deprecated. Please use the ExternalReference relation instead.");
        // Handle assemblyData
        if (domain == "ASSEMBLY" && v.contains("assemblyData"))
            model->set_data(v["assemblyData"]);
        else if (domain == "SOFTWARE" && v.contains("softwareData"))
            model->set_data(v["softwareData"]);
        else if (domain == "ELECTRONICS" && v.contains("electronicsData"))
            model->set_data(v["electronicsData"]);
        else if (domain == "MECHANICS" && v.contains("mechanicsData"))
            model->set_data(v["mechanicsData"]);
        else if (domain == "COMPUTATION" && v.contains("computationData"))
            model->set_data(v["computationData"]);
        else if (v.contains("data"))
            model->set_data(v["data"]);

        // Handling subcomponents/parts and their connections
        bool error_occurred = false;
        std::vector<ComponentPtr> parts;
        if (v.contains("components"))
        {
            // Handling parts
            if (v["components"].contains("nodes"))
            {
                for (const auto &part : v["components"]["nodes"])
                {
                    const std::string partName(part["name"].get<std::string>());
                    const std::string partModelName(part["model"]["name"].get<std::string>());
                    const std::string partModelDomain(part["model"]["domain"].get<std::string>());
                    const std::string partModelVersion(part["model"]["version"].get<std::string>());

                    // First lookup component model in cache
                    ComponentModelPtr partModel{nullptr};
                    const std::string lookup(partModelName + partModelDomain + partModelVersion);
                    if (part_models.count(lookup))
                    {
                        partModel = part_models.at(lookup);
                    }
                    else
                    {
                        // ... if not found, call provided function to search for a matching part model
                        ComponentModel dummy;
                        dummy.set_properties(part["model"]);
                        partModel = std::static_pointer_cast<ComponentModel>(registry->load_by_uri(dummy.uri()));
                        // and place it into cache
                        if (partModel)
                        {
                            part_models[lookup] = partModel;
                        }
                    }
                    // If the model has not been found, ignore it
                    if (!partModel)
                    {
                        std::cerr << "ComponentModel::import_from_basic_model: Could not load model " + partModelName + " for part " + partName << "\n";
                        error_occurred = true;
                        break;
                    }

                    // Create the part (and set all inner facts to be empty, not unknown)
                    ComponentPtr p = partModel->instantiate(model, partName, true);
                    if (!p)
                    {
                        std::cerr << "ComponentModel::import_from_basic_model: Could instantiate part " + partName << "\n";
                        error_occurred = true;
                        break;
                    }
                    // Set alias (if given)
                    if (part.contains("alias"))
                    {
                        p->set_alias(part["alias"]);
                    }
                    // Set interface aliases
                    if (part.contains("interface_aliases"))
                    {
                        for (const auto&[ifName, ifAlias] : part["interface_aliases"].items())
                        {
                            xtypes::InterfacePtr pi = p->get_interface(ifName);
                            if (!pi)
                            {
                                std::cerr << "ComponentModel::import_from_basic_model: Could not find part interface " + ifName + " to set alias\n";
                                error_occurred = true;
                                continue;
                            }
                            pi->set_alias(ifAlias);
                        }
                    }
                    parts.push_back(p);
                }
            }

            // Handle connections
            if (v["components"].contains("edges"))
            {
                for (const auto &conn : v["components"]["edges"])
                {
                    // Check and parse info for source part
                    if (!conn.contains("from"))
                    {
                        std::cerr << "XType::import_from_basic_model: 'from' entry missing in " << conn.dump() << "\n";
                        error_occurred = true;
                        break;
                    }
                    if (!conn["from"].contains("name"))
                    {
                        std::cerr << "XType::import_from_basic_model: 'name' entry missing in " << conn["from"].dump() << "\n";
                        error_occurred = true;
                        break;
                    }
                    const std::string fromPartName(conn["from"]["name"].get<std::string>());
                    if (!conn["from"].contains("interface"))
                    {
                        std::cerr << "XType::import_from_basic_model: interface entry in 'from' for part " << fromPartName << " is missing.\n";
                        error_occurred = true;
                        break;
                    }
                    const std::string fromPartInterfaceName(conn["from"]["interface"].get<std::string>());
                    std::string fromPartInterfaceDomain(std::string("NOT_SET"));
                    if (conn["from"].contains("domain"))
                    {
                        fromPartInterfaceDomain = conn["from"]["domain"].get<std::string>();
                    }

                    // Check and parse info for destination part
                    if (!conn.contains("to"))
                    {
                        std::cerr << "XType::import_from_basic_model: 'to' entry missing in " << conn.dump() << "\n";
                        error_occurred = true;
                        break;
                    }
                    if (!conn["to"].contains("name"))
                    {
                        std::cerr << "XType::import_from_basic_model: 'name' entry missing in " << conn["to"].dump() << "\n";
                        error_occurred = true;
                        break;
                    }
                    const std::string toPartName(conn["to"]["name"].get<std::string>());
                    if (!conn["to"].contains("interface"))
                    {
                        std::cerr << "XType::import_from_basic_model: interface entry in 'to' for part " << toPartName << " is missing.\n";
                        error_occurred = true;
                        break;
                    }
                    const std::string toPartInterfaceName(conn["to"]["interface"].get<std::string>());
                    std::string toPartInterfaceDomain(std::string("NOT_SET"));
                    if (conn["to"].contains("domain"))
                    {
                        toPartInterfaceDomain = conn["to"]["domain"].get<std::string>();
                    }

                    // Resolve source part, source interface, destination part and destination interface to connect them
                    // First resolve source part and interface
                    ComponentPtr fromPart{nullptr}, toPart{nullptr};
                    InterfacePtr fromInterface{nullptr}, toInterface{nullptr};
                    // Find source and destination parts first ...
                    for (const auto part : parts)
                    {
                        if (part->get_name() == fromPartName)
                            fromPart = part;
                        if (part->get_name() == toPartName)
                            toPart = part;
                        if (fromPart && toPart)
                            break;
                    }
                    if (!fromPart)
                    {
                        std::cerr << "ComponentModel::import_from_basic_model(): Could not find source part " + fromPartName << "\n";
                        error_occurred = true;
                        break;
                    }
                    if (!toPart)
                    {
                        std::cerr << "ComponentModel::import_from_basic_model(): Could not find target part " + toPartName << "\n";
                        error_occurred = true;
                        break;
                    }
                    // ... then the interfaces
                    for (const auto &[fpi, _] : fromPart->get_facts("interfaces"))
                    {
                        // TODO: Should we check the interface domains?
                        // if fromPartInterfaceDomain != fromPartInterface.domain:
                        //    continue
                        InterfacePtr fromPartInterface = std::static_pointer_cast<Interface>(fpi.lock());
                        if (fromPartInterface->get_name() == fromPartInterfaceName)
                        {
                            fromInterface = std::move(fromPartInterface);
                            break;
                        }
                    }
                    if (!fromInterface)
                    {
                        std::cerr << "ComponentModel::import_from_basic_model(): Could not find source interface " + fromPartInterfaceName + " at part " + fromPartName << "\n";
                        error_occurred = true;
                        break;
                    }
                    for (const auto &[tpi, _] : toPart->get_facts("interfaces"))
                    {
                        // TODO: Should we check the interface domains?
                        // if toPartInterfaceDomain != toPartInterface.domain:
                        //    continue
                        InterfacePtr toPartInterface = std::static_pointer_cast<Interface>(tpi.lock());
                        if (toPartInterface->get_name() == toPartInterfaceName)
                        {
                            toInterface = std::move(toPartInterface);
                            break;
                        }
                    }
                    if (!toInterface)
                    {
                        std::cerr << "ComponentModel::import_from_basic_model(): Could not find target interface " + toPartInterfaceName + " at part " + toPartName << "\n";
                        error_occurred = true;
                        break;
                    }
                    // Connect src and target interface
                    // Now that we have a global registry, a connection could already be existent!
                    error_occurred = !fromInterface->connected_to(toInterface, conn);
                    if (error_occurred)
                    {
                        std::cerr << "ComponentModel::import_from_basic_model(): Could not connect " + fromPartInterfaceName + " and " + toPartInterfaceName << "\n";
                        break;
                    }
                }
            }

            // Handle configuration
            if (v["components"].contains("configuration"))
            {
                if (v["components"]["configuration"].contains("nodes"))
                {
                    for (const auto &config : v["components"]["configuration"]["nodes"])
                    {
                        // Sometimes old DROCK stuff produces empty config entries
                        if (!config.contains("name"))
                            continue;
                        const std::string partName(config["name"].get<std::string>());
                        bool found = false;
                        for (const auto p : parts)
                        {
                            if (p->get_name() != partName)
                                continue;
                            found = true;
                            p->set_property("configuration", config);
                        }
                        if (!found)
                        {
                            std::cerr << "ComponentModel::import_from_basic_model(): Could not find part " + partName + " in model " + model->get_name() << "\n";
                            error_occurred = true;
                            break;
                        }
                    }
                }
                if (v["components"]["configuration"].contains("edges"))
                {
                    for (const auto &config : v["components"]["configuration"]["edges"])
                    {
                        // Sometimes old DROCK stuff produces empty config entries
                        if (!config.contains("name"))
                            continue;
                        std::string connectionName = config["name"].get<std::string>();
                        bool found = false;
                        for (const auto &p : parts)
                        {
                            for (const auto &[pi, _] : p->get_facts("interfaces"))
                            {
                                XTypeCPtr src_interface = pi.lock();
                                for (const auto &[o, props] : src_interface->get_facts("others"))
                                {
                                    if (props["name"].get<std::string>() != connectionName)
                                        continue;
                                    found = true;
                                    // NOTE: We reinsert the same fact to update the edge properties
                                    XTypeCPtr dst_interface = o.lock();
                                    nl::json new_props = props;
                                    new_props["configuration"] = config;
                                    src_interface->add_fact("others", dst_interface, new_props);
                                }
                            }
                        }
                        if (!found)
                        {
                            std::cerr << "ComponentModel::import_from_basic_model(): Could not find connection " + connectionName + " in model " + model->get_name() << std::endl;
                            error_occurred = true;
                            break;
                        }
                    }
                }
            }
        }

        // Handling interfaces
        if (v.contains("interfaces"))
        {
            for (const auto &modelIf : v["interfaces"])
            {
                // First, create the interface model (by type and domain)
                std::string ifModelName = std::string("UNKNOWN");
                if (modelIf.contains("type"))
                {
                    ifModelName = modelIf["type"].get<std::string>();
                }
                // For the interfaces we set the default domain to the domain of the owning model. That will prevent issues with compatibility checking.
                std::string ifModelDomain = model->get_domain();
                if (modelIf.contains("domain"))
                {
                    ifModelDomain = modelIf["domain"].get<std::string>();
                }
                // First create a dummy and try to load it before creating it new
                InterfaceModel dummy;
                dummy.set_properties({{"name", ifModelName},
                                         {"domain", ifModelDomain}});
                InterfaceModelPtr ifModel = std::static_pointer_cast<InterfaceModel>(registry->load_by_uri(dummy.uri()));
                if (!ifModel)
                {
                    // create a new interface model
                    ifModel = registry->instantiate<InterfaceModel>();
                    ifModel->set_properties(dummy.get_properties());
                }
                // Then we instantiate from it
                InterfacePtr interface = ifModel->instantiate(model, modelIf["name"].get<std::string>());
                // ... and update the properties
                interface->set_properties(modelIf, false);

                // Handle alias interfaces
                // Set original to empty first
                interface->set_unknown_fact_empty("original");
                if (modelIf.contains("linkToNode") && modelIf.contains("linkToInterface"))
                {
                    const std::string partName(modelIf["linkToNode"].get<std::string>());
                    const std::string partInterfaceName(modelIf["linkToInterface"].get<std::string>());
                    // Find part
                    bool found = false;
                    for (const auto part : parts)
                    {
                        if (part->get_name() != partName)
                            continue;
                        found = true;
                        // Found part: find interface next
                        InterfacePtr partInterface{nullptr};
                        for (const auto &[i, _] : part->get_facts("interfaces"))
                        {
                            InterfacePtr pi = std::static_pointer_cast<Interface>(i.lock());
                            if (pi->get_name() == partInterfaceName)
                            {
                                partInterface = std::move(pi);
                                break;
                            }
                        }
                        if (!partInterface)
                        {
                            std::cerr << "ComponentModel::importFromBasicModelJSON(): Could not find internal interface " + partInterfaceName + " of part " + partName << std::endl;
                            break;
                        }
                        // Found matching pair
                        interface->alias_of(partInterface);
                    }
                    if (!found)
                    {
                        std::cerr << "ComponentModel::import_from_basic_model(): Could not find part " + partName + " in model " + model->get_name() << std::endl;
                        error_occurred = true;
                        break;
                    }
                }
            }
        }

        if (v.contains("dynamic_interfaces"))
        {
            for (const auto &modelIf : v["dynamic_interfaces"])
            {
                // First, create the interface model (by type and domain)
                std::string ifModelName = std::string("UNKNOWN");
                if (modelIf.contains("type"))
                {
                    ifModelName = modelIf["type"].get<std::string>();
                }
                // For the dynamic interfaces we set the default domain to the domain of the owning model. That will prevent issues with compatibility checking.
                std::string ifModelDomain = model->get_domain();
                if (modelIf.contains("domain"))
                {
                    ifModelDomain = modelIf["domain"].get<std::string>();
                }
                // First create a dummy and try to load it before creating it new
                InterfaceModel dummy;
                dummy.set_properties({{"name", ifModelName},
                                         {"domain", ifModelDomain}});
                InterfaceModelPtr ifModel = std::static_pointer_cast<InterfaceModel>(registry->load_by_uri(dummy.uri()));
                if (!ifModel)
                {
                    // create a new interface model
                    ifModel = registry->instantiate<InterfaceModel>();
                    ifModel->set_properties(dummy.get_properties());
                }
                // Then we instantiate from it
                DynamicInterfacePtr interface = ifModel->instantiate_dynamic(model); // HERE
                // ... and update the properties
                interface->set_properties(modelIf, false);
            }
        }

        if (error_occurred)
        {
            std::cerr << "ComponentModel::import_from_basic_model(): Error ocurred while parsing " << domain << " " << name << " " << version << ". Skipping it.\n";
            continue;
        }

        // Add VALID component model to result
        result.push_back(std::move(model));
    }
    return result;
}

InterfacePtr xtypes::ComponentModel::export_inner_interface(xtypes::InterfacePtr inner_interface, const bool& with_empty_facts)
{
    const auto inner_interface_name = inner_interface->get_name();
    const auto parent = std::static_pointer_cast<Component>(inner_interface->get_facts("parent")[0].target.lock());

    // Check if inner interface is already exported
    InterfacePtr outer_interface{nullptr};
    for (const auto& modelIf : this->get_interfaces())
    {
        if (!modelIf->has_relation("original") || !modelIf->has_same_type(inner_interface)) {
            continue;
        }
        for (const auto &[i, _] : modelIf->get_facts("original"))
        {
            const InterfacePtr original_iface(std::static_pointer_cast<Interface>(i.lock()));
            if (original_iface->uri() == inner_interface->uri())
            {
                outer_interface = std::move(modelIf);
                std::cout << "Found existing interface " << outer_interface->get_name() << " for " << this->get_name() << "\n";
                break;
            }
        }
    }

    if (outer_interface)
    {
        outer_interface->alias_of(inner_interface);
        return outer_interface;
    }

    // Create new interface as a clone of the inner interface
    XTypeRegistryPtr reg = registry.lock();
    if (!reg)
    {
        throw std::invalid_argument("ComponentModel::export_inner_interface: no registry");
    }
    const auto new_name = parent->get_name() + ":" + inner_interface_name;
    std::cout << "Creating new interface " << new_name << " for " << this->get_name() << "\n";
    
    const InterfaceModelPtr model(inner_interface->get_type());
    outer_interface = std::static_pointer_cast<Interface>(reg->instantiate<Interface>());

    if (with_empty_facts)
    {
        outer_interface->set_all_unknown_facts_empty();
    }
    outer_interface->set_properties(inner_interface->get_properties());
    outer_interface->set_property("name", new_name);
    outer_interface->child_of(shared_from_this());
    outer_interface->alias_of(inner_interface);
    outer_interface->instance_of(model);
    return outer_interface;
}

// Annotates the ComponentModel with an optional or needed ExternalReference. Calls _ComponentModel::add_external_references internally
void xtypes::ComponentModel::annotate_with(const ExternalReferencePtr reference, const bool& optional)
{
    nl::json edge_properties = {{"optional", optional}};
    this->add_external_references(reference, edge_properties);
}

void xtypes::ComponentModel::add_parts(xtypes::ComponentCPtr xtype, const nl::json& props)
{
    // Add your advanced code here
    if (!this->get_can_have_parts())
    {
        throw std::runtime_error("xtypes::ComponentModel::add_parts: Trying to add parts to a ComponentModel, that is defined to not have parts!");
    }
    // NOTE: We cannot call add_parts() here, we have to call inversly because the part uri might not be valid yet
    xtype->add_whole(std::static_pointer_cast<ComponentModel>(shared_from_this()));
}

void xtypes::ComponentModel::add_abstracts(xtypes::ComponentModelCPtr xtype, const nl::json& props)
{
    // Add your advanced code here
    if (!this->is_valid_implementation(xtype))
    {
        throw std::runtime_error("xtypes::ComponentModel::add_abstracts(): This ComponentModel is not a valid implementation of superclass");
    }
    // Finally call the overridden method
    this->_ComponentModel::add_abstracts(xtype, props);
}

void xtypes::ComponentModel::add_implementations(xtypes::ComponentModelCPtr xtype, const nl::json& props)
{
    // Add your advanced code here
    if (!xtype->is_valid_implementation(std::static_pointer_cast<ComponentModel>(shared_from_this())))
    {
        throw std::runtime_error("xtypes::ComponentModel::add_implementations(): Given xtype is not a valid implementation for us");
    }
    // Finally call the overridden method
    this->_ComponentModel::add_implementations(xtype, props);
}

bool xtypes::ComponentModel::can_configure(const ComponentModelPtr xtype) 
{
    return (this->get_property("domain") == "SOFTWARE" && xtype->get_property("domain") == "ASSEMBLY");
}

void xtypes::ComponentModel::add_configured_for(xtypes::ComponentModelCPtr xtype, const nl::json &props)
{
    if (!can_configure(xtype))
    {
        throw std::runtime_error("xtypes::ComponentModel::add_configured_for(): Given xtype is not valid for configuration");
    }
    // Finally call the overridden method
    this->_ComponentModel::add_configured_for(xtype, props);
}

void xtypes::ComponentModel::add_deployables(xtypes::ComponentModelCPtr xtype, const nl::json &props)
{
    if (!xtype->can_configure(std::static_pointer_cast<ComponentModel>(shared_from_this())))
    {
        throw std::runtime_error("xtypes::ComponentModel::add_deployables(): Given xtype is not valid for deployment");
    }
    // Finally call the overridden method
    this->_ComponentModel::add_deployables(xtype, props);
}
std::vector<ComponentModelPtr> xtypes::ComponentModel::get_configured_for()
{
    std::vector<ComponentModelPtr> configuredFor;
    auto facts = this->get_facts("configured_for");
    configuredFor.reserve(facts.size());
    for (const auto &[fact, _] : facts)
    {
        const ComponentModelPtr config = std::static_pointer_cast<ComponentModel>(fact.lock());
        if (config)
        {
            configuredFor.push_back(config);
        }
    }
    return configuredFor;
}

void xtypes::ComponentModel::remove_configured_for(ComponentModelPtr hardware)
{
    // Unlink this SOFTWARE with hardware ASSEMBLY
    hardware->remove_fact("deployables", std::static_pointer_cast<ComponentModel>(shared_from_this()));

}

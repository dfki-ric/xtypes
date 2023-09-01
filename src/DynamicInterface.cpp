#include "DynamicInterface.hpp"

// Including used XType classes

#include <iostream>

#include "Component.hpp"
#include "ComponentModel.hpp"
#include "Interface.hpp"
#include "InterfaceModel.hpp"

using namespace xtypes;

// Constructor
xtypes::DynamicInterface::DynamicInterface(const std::string& classname) : _DynamicInterface(classname)
{
    // NOTE: Properties and relations have been created in _DynamicInterface constructor
}

// Static identifier
const std::string xtypes::DynamicInterface::classname = "xtypes::DynamicInterface";

// Method implementations
// This function returns the domain of the interface
std::string xtypes::DynamicInterface::get_domain()
{
    return (this->get_type()->get_domain());
}

// Returns the model of which this interface has been instantiated from
InterfaceModelPtr xtypes::DynamicInterface::get_type()
{
    return (std::static_pointer_cast<InterfaceModel>(this->get_facts("model")[0].target.lock()));
}

// This function instantiates a new instance of the dynamic interface for a component
InterfacePtr xtypes::DynamicInterface::instantiate(const ComponentPtr for_component, const std::string& name, const std::string& direction, const std::string& multiplicity, const bool& with_empty_facts)
{
    // Check if the parent component model of the dynamic interface matches the model of the component we want to add an interface for
    const xtypes::ComponentModelPtr interface_parent(std::static_pointer_cast<ComponentModel>(this->get_facts("parent")[0].target.lock()));
    const xtypes::ComponentModelPtr component_parent(std::static_pointer_cast<ComponentModel>(for_component->get_facts("model")[0].target.lock()));
    if (interface_parent->uuid() != component_parent->uuid())
    {
        throw std::invalid_argument("xtypes::DynamicInterface::instantiate(): Component model of dynamic interface does not match the model of the component");
    }

    XTypeRegistryPtr reg = registry.lock();
    if (!reg)
    {
        throw std::runtime_error(this->get_classname()+"::instantiate(): No registry");
    }

    // Create a new interface
    const xtypes::InterfaceModelPtr interface_model(std::static_pointer_cast<InterfaceModel>(this->get_facts("model")[0].target.lock()));
    InterfacePtr interf = reg->instantiate<Interface>();
    if (with_empty_facts)
    {
        interf->set_all_unknown_facts_empty();
    }
    interf->set_properties(this->get_properties());
    interf->set_name(name);
    interf->set_direction((direction == "DIRECTION_NOT_SET") ? this->get_direction() : direction);
    interf->set_multiplicity((multiplicity == "MULTIPLICITY_NOT_SET") ? this->get_multiplicity() : multiplicity);
    interf->instance_of(interface_model);
    interf->child_of(for_component);
    return interf;
}

void xtypes::DynamicInterface::instance_of(const InterfaceModelPtr model)
{
    this->add_model(model);
}

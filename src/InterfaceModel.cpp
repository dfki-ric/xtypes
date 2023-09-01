
#include "InterfaceModel.hpp"

// Including used XType classes
#include "Interface.hpp"
#include "DynamicInterface.hpp"
#include "ComponentModel.hpp"


using namespace xtypes;

// Constructor
xtypes::InterfaceModel::InterfaceModel(const std::string& classname) : _InterfaceModel(classname)
{
    // NOTE: Properties and relations have been created in _InterfaceModel constructor
}

// Static identifier
const std::string xtypes::InterfaceModel::classname = "xtypes::InterfaceModel";

// Method implementations
//
// This function instantiates a new interface instance for a component model
InterfacePtr xtypes::InterfaceModel::instantiate(const ComponentModelPtr for_component_model, const std::string& name, const std::string& direction, const std::string& multiplicity, const bool& with_empty_facts)
{
    if (!for_component_model)
    {
        throw std::invalid_argument(this->get_classname() + "::instantiate(): need a component model for interface instantiation");
    }
    XTypeRegistryPtr reg = registry.lock();
    if (!reg)
    {
        throw std::runtime_error(this->get_classname() + "::instantiate(): No registry");
    }
    InterfacePtr interface = reg->instantiate<Interface>();
    if (!interface)
    {
        throw std::runtime_error(this->get_classname() + "::instantiate(): Registry could not instantiate interface");
    }
    if (with_empty_facts)
    {
        interface->set_all_unknown_facts_empty();
    }
    interface->set_properties(this->get_properties(), false);
    // Override name if given
    if (!name.empty())
    {
        interface->set_name(name);
    }
    interface->set_direction((direction == "DIRECTION_NOT_SET") ? this->get_direction() : direction);
    interface->set_multiplicity((multiplicity == "MULTIPLICITY_NOT_SET") ? this->get_multiplicity() : multiplicity);
    for_component_model->has(interface);
    interface->instance_of(std::static_pointer_cast<InterfaceModel>(shared_from_this()));
    return interface;
}

// This function instantiates a new dynamic interface instance for a component model
DynamicInterfacePtr xtypes::InterfaceModel::instantiate_dynamic(const ComponentModelPtr for_component_model, const std::string& direction, const std::string& multiplicity, const bool& with_empty_facts)
{
    if (!for_component_model)
    {
        throw std::runtime_error("xtypes::InterfaceModel::instantiate: need a component model for interface instantiation");
    }
    XTypeRegistryPtr reg = registry.lock();
    if (!reg)
    {
        throw std::runtime_error(this->get_classname() + "::instantiate(): No registry");
    }
    DynamicInterfacePtr interface = reg->instantiate<DynamicInterface>();
    if (with_empty_facts)
    {
        interface->set_all_unknown_facts_empty();
    }
    interface->set_properties(this->get_properties(), false);
    interface->set_direction((direction == "DIRECTION_NOT_SET") ? this->get_direction() : direction);
    interface->set_multiplicity((multiplicity == "MULTIPLICITY_NOT_SET") ? this->get_multiplicity() : multiplicity);
    for_component_model->has(interface);
    interface->instance_of(std::static_pointer_cast<InterfaceModel>(shared_from_this()));
    return interface;
}

// Returns all the direct superclasses of this model
std::vector<InterfaceModelPtr> xtypes::InterfaceModel::get_types()
{
    std::vector<InterfaceModelPtr> result;
    for (const auto& [im, _] : this->get_facts("model"))
    {
        const xtypes::InterfaceModelPtr model(std::static_pointer_cast<InterfaceModel>(im.lock()));
        result.push_back(model);
    }
    return result;
}

// This function sets the superclass of the InterfaceModel and updates the type property accordingly
void xtypes::InterfaceModel::subclass_of(const InterfaceModelPtr superclass)
{
    this->add_model(superclass);
}

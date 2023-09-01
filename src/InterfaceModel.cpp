
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
    // Here, we register ourselves and the xtypes we use to our registry
    registry->register_class<ComponentModel>();

    registry->register_class<DynamicInterface>();

    registry->register_class<Interface>();

    registry->register_class<InterfaceModel>();

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
        throw std::invalid_argument("xtypes::InterfaceModel::instantiate: need a component model for interface instantiation");
    }

    InterfacePtr interface = registry->instantiate<Interface>();
    if (with_empty_facts)
    {
        interface->set_all_unknown_facts_empty();
    }
    interface->set_properties(this->get_properties(), false);
    // making XType inherrit from std::enable_shared_from_this provide us with
    // ... a new method called shared_from_this() which we can use to create a shared_ptr
    // ... from a 'this' pointer whithout freeing the same memory location twice (double free or corruption (out))
    interface->instance_of(std::dynamic_pointer_cast<InterfaceModel>(shared_from_this()));
    // Override name if given
    if (!name.empty())
    {
        interface->set_name(name);
    }
    interface->set_direction((direction == "DIRECTION_NOT_SET") ? this->get_direction() : direction);
    interface->set_multiplicity((multiplicity == "MULTIPLICITY_NOT_SET") ? this->get_multiplicity() : multiplicity);
    for_component_model->has(interface);
    return interface;
}

// This function instantiates a new dynamic interface instance for a component model
DynamicInterfacePtr xtypes::InterfaceModel::instantiate_dynamic(const ComponentModelPtr for_component_model, const std::string& direction, const std::string& multiplicity, const bool& with_empty_facts)
{
    if (!for_component_model)
    {
        throw std::runtime_error("xtypes::InterfaceModel::instantiate: need a component model for interface instantiation");
    }

    DynamicInterfacePtr interface = registry->instantiate<DynamicInterface>();
    if (with_empty_facts)
    {
        interface->set_all_unknown_facts_empty();
    }
    interface->set_properties(this->get_properties(), false);
    // making XType inherrit from std::enable_shared_from_this provide us with
    // ... a new method called shared_from_this() which we can use to create a shared_ptr
    // ... from a 'this' pointer whithout freeing the same memory location twice (double free or corruption (out))
    interface->instance_of(std::dynamic_pointer_cast<InterfaceModel>(shared_from_this()));
    interface->set_direction((direction == "DIRECTION_NOT_SET") ? this->get_direction() : direction);
    interface->set_multiplicity((multiplicity == "MULTIPLICITY_NOT_SET") ? this->get_multiplicity() : multiplicity);
    for_component_model->has(interface);
    return interface;
}

// Returns all the superclasses of this model (transitive closure over superclass_of relation)
std::vector<InterfaceModelPtr> xtypes::InterfaceModel::get_types() const
{
    std::vector<InterfaceModelPtr> result;
    for (const auto& [im, _] : this->get_facts("model"))
    {
        const xtypes::InterfaceModelPtr model(std::static_pointer_cast<InterfaceModel>(im.lock()));
        // TODO: Call model->get_types()
        result.push_back(model);
    }
    return result;
}

// This function sets the superclass of the InterfaceModel and updates the type property accordingly
void xtypes::InterfaceModel::subclass_of(const InterfaceModelPtr superclass)
{
    this->add_model(superclass);
}

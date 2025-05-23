#include "Module.hpp"

// Including used XType classes
#include "ComponentModel.hpp"

#include <inja/inja.hpp>

using namespace xtypes;

// Constructor
xtypes::Module::Module(const std::string& classname) : _Module(classname)
{
    // NOTE: Properties and relations have been created in _Module constructor
}

// Static identifier
const std::string xtypes::Module::classname = "xtypes::Module";

// Method implementations
// Returns true if this Module hasn't any parts
bool xtypes::Module::is_atomic()
{
    return (this->get_facts("parts").size() == 0);
}

// Search and return a part of the module with the given name
ModulePtr xtypes::Module::get_part(const std::string& name)
{
    for (const auto &[p,_] : this->get_facts("parts"))
    {
        const ModulePtr part(std::static_pointer_cast<Module>(p.lock()));
        if (part->get_name() == name)
            return part;
    }
    return nullptr;
}

// This function marks the module as being part of another module
void xtypes::Module::part_of(const ModulePtr whole)
{
    this->add_whole(whole);
}

// This function applies any pending configuration updates (except global variables) inside the module hierarchy (config_overrides overwrites lower level configuration values)
void xtypes::Module::configure(const nl::json& config_overrides)
{
    nl::json new_config_overrides = config_overrides;
    const std::string name_or_alias(this->get_alias().empty() ? this->get_name() : this->get_alias());
    nl::json current_config = this->get_configuration();
    // Apply config_overrides to our config if our name is present
    // If present, remove that entry from config_overrides
    if (new_config_overrides.contains(name_or_alias) && new_config_overrides[name_or_alias].is_object())
    {
        current_config.update(new_config_overrides[name_or_alias], true); // deep_merge
        new_config_overrides.erase(name_or_alias);
    }
    // Check if a 'submodel' statement is present in our config (possibly updated before)
    // If it is, remove it and put it into the config_overrides (do NOT resolve inner submodel statements)
    if (current_config.contains("submodel"))
    {
        for (const auto& entry : current_config["submodel"])
        {
            new_config_overrides[entry["name"].get<std::string>()] = entry;
        }
        current_config.erase("submodel");
    }
    // Update our configuration
    this->set_configuration(current_config);
    // Call configure with the (possibly updates config_overrides) on all our submodules
    for (const auto& [p, _] : this->get_facts("parts"))
    {
        const xtypes::ModulePtr part(std::static_pointer_cast<Module>(p.lock()));
        part->configure(new_config_overrides);
    }
}

// This functions will go through this Module and it's sub-Modules and resolve the global_variables in there configuration
void xtypes::Module::apply_global_variables(const nl::json& global_variables)
{
    nl::json vars = this->get_global_variables(global_variables);
    // FIXME: We cannot return here! It may be that there are still global_variables pending BELOW the current level!!!
    if (!vars.is_object() || (vars.size() < 1))
        return;
    std::string configuration = inja::render(this->get_configuration().dump(), vars);
    this->set_configuration(nl::json::parse(configuration));
    for (const auto& [p, _] : this->get_facts("parts"))
    {
        const xtypes::ModulePtr part(std::static_pointer_cast<Module>(p.lock()));
        part->apply_global_variables(vars);
    }
}

// Merges the global variables defined on this Module level into the given global_variables without overriding them, and returns them
nl::json xtypes::Module::get_global_variables(const nl::json& global_variables)
{
    assert (this->get_facts("model").size() == 1); // per definitionem
    const ConstComponentModelCPtr model(std::static_pointer_cast<ComponentModel>(
        this->get_facts("model").at(0).target.lock()
    ));
    // HW 2023-06-22: This has to be replaced by the global_variables property
    nl::json merged_vars;
    if (!model->get_data().is_null() && model->get_data().contains("globalVariables"))
        merged_vars = model->get_data().at("globalVariables");
    else
        merged_vars = nl::json(nl::json::value_t::object);
    // HW 2023-06-22: At this point we need to integrate vars that are contained by external_references
    merged_vars.update(global_variables);
    return merged_vars;
}

// Overrides for setters of properties

// Overrides for relation setters

void xtypes::Module::add_whole(xtypes::ModuleCPtr xtype, const nl::json& props)
{
    // Add your advanced code here
    if (this->has_facts("model"))
    {
        const std::vector<xtypes::Fact> &the_facts(this->get_facts("model"));
        if ((the_facts.size() > 0) && the_facts[0].target.lock()->uuid() == xtype->uuid())
            throw std::invalid_argument("xtypes::Module::add_whole(): Cannot be part of my own model");
    }
    // Finally call the overridden method
    this->_Module::add_whole(xtype, props);
}

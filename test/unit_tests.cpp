#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <iostream>
// Include XTypes
#include <xtypes_generator/utils.hpp>

#include "ComponentModel.hpp"
#include "DynamicInterface.hpp"
#include "InterfaceModel.hpp"
#include "Interface.hpp"
#include "Component.hpp"
#include "Module.hpp"
#include "ProjectRegistry.hpp"
#include "git_wrapper.hpp"


static std::once_flag onceFlag;

using namespace xtypes;

TEST_CASE("Test InterfaceModel class interface", "InterfaceModel")
{
    XTypeRegistryPtr pr = std::make_shared<ProjectRegistry>();

    SECTION("instantiate")
    {
        ComponentModelPtr cm = pr->instantiate<ComponentModel>();
        InterfaceModelPtr im = pr->instantiate<InterfaceModel>();
        InterfacePtr a = im->instantiate(cm, "a");
        REQUIRE(a != nullptr);
        REQUIRE(a->get_name() == "a");
        InterfacePtr b = im->instantiate(cm, "b");
        REQUIRE(b->get_name() == "b");
        REQUIRE(b != nullptr);
        REQUIRE(a->uri() != b->uri());
        REQUIRE(cm->get_facts("interfaces").size() == 2);
    }
    
    pr->clear();

    SECTION("instantiate_dynamic")
    {
        ComponentModelPtr cm = pr->instantiate<ComponentModel>();
        InterfaceModelPtr im = pr->instantiate<InterfaceModel>();
        im->set_name("some type");
        DynamicInterfacePtr a = im->instantiate_dynamic(cm);
        REQUIRE(a != nullptr);
        REQUIRE(a->get_type()->get_property("name") == im->get_name());
        REQUIRE(cm->get_facts("dynamic_interfaces").size() == 1);
    }
}



TEST_CASE("Test Interface class interface", "Interface")
{
    XTypeRegistryPtr pr = std::make_shared<ProjectRegistry>();

    SECTION("alias_of")
    {
        InterfaceModelPtr im = pr->instantiate<InterfaceModel>();
        ComponentModelPtr cm = pr->instantiate<ComponentModel>();
        InterfacePtr i = im->instantiate(cm,  "a",  "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        InterfacePtr i1 = im->instantiate(cm, "b", "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        i->add_original(i1);
        REQUIRE(i->get_facts("original").size() == 1);
    }

    pr->clear();

    SECTION("child_of")
    {
        XTypePtr xtype = pr->instantiate<XType>();
        InterfacePtr interface = pr->instantiate<Interface>();
        ComponentModelPtr component_model = pr->instantiate<ComponentModel>();
        REQUIRE_THROWS(interface->child_of(xtype));
        interface->child_of(component_model);
        REQUIRE(interface->get_facts("parent").size() == 1);
    }

    // TODO: instance_of(const InterfaceModelPtr model);
    
    pr->clear();

    // TODO: is_connectable_to(const InterfacePtr other);
    // TODO: is_compatible_with(const InterfacePtr other);

    SECTION("connected_to/disconnect")
    {
        InterfaceModelPtr im = pr->instantiate<InterfaceModel>();
        ComponentModelPtr cm = pr->instantiate<ComponentModel>();
        InterfacePtr i = im->instantiate(cm,  "a",  "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        InterfacePtr i1 = im->instantiate(cm, "b", "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        InterfacePtr i2 = im->instantiate(cm, "c", "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        REQUIRE(i->connected_to(i1, {}) == true);
        REQUIRE(i->connected_to(i2, {}) == true);
        REQUIRE(i->get_facts("others").size() == 2);
        REQUIRE(i1->get_facts("from_others").size() == 1);
        REQUIRE(i2->get_facts("from_others").size() == 1);
        // TODO: Check connection with properties
        i->disconnect();
        REQUIRE(i->get_facts("others").size() == 0);
        REQUIRE(i1->get_facts("from_others").size() == 0);
        REQUIRE(i2->get_facts("from_others").size() == 0);
    }
    
    pr->clear();

    SECTION("get_dynamic_interface")
    {
        // First setup a component model with a dynamic interface
        ComponentModelPtr cm = pr->instantiate<ComponentModel>();
        cm->set_all_unknown_facts_empty();
        InterfaceModelPtr im = pr->instantiate<InterfaceModel>();
        im->set_name("some type");
        im->set_all_unknown_facts_empty();
        DynamicInterfacePtr a = im->instantiate_dynamic(cm);
        REQUIRE(a != nullptr);
        REQUIRE(a->get_type()->get_property("name") == im->get_name());
        REQUIRE(cm->get_facts("dynamic_interfaces").size() == 1);
        // Then instantiate a component
        ComponentModelPtr cm2 = pr->instantiate<ComponentModel>();
        cm2->set_name("a whole");
        cm2->set_all_unknown_facts_empty();
        ComponentPtr c = cm->instantiate(cm2, "X", false);
        REQUIRE(!c->has_facts("interfaces"));
        // Dynamic interfaces have to be instantiated 'by hand'
        // TODO: At component or component model level there should be nice functions to get dynamic interfaces
        std::string name = "x";
        for (const auto &[dynIf, _] : cm->get_facts("dynamic_interfaces"))
        {
            DynamicInterfacePtr di = std::static_pointer_cast<DynamicInterface>(dynIf.lock());
            InterfacePtr i = di->instantiate(c, name);
            REQUIRE(i->get_dynamic_interface() != nullptr);
            name = name + "x";
        }
        // In the end, the component should have a real interface because the model had one dynamic interface
        REQUIRE(cm->get_facts("dynamic_interfaces").size() == c->get_facts("interfaces").size());
    }
}

TEST_CASE("Test Component class interface", "Component")
{
    XTypeRegistryPtr pr = std::make_shared<ProjectRegistry>();

    // TODO: instance_of(ComponentModelCPtr model)
    // TODO: part_of(ComponentModelCPtr whole)
    // TODO: has(InterfaceCPtr interface)

    SECTION("get_interface")
    {
        InterfaceModelPtr im = pr->instantiate<InterfaceModel>();
        ComponentModelPtr cm = pr->instantiate<ComponentModel>();
        InterfacePtr i = im->instantiate(cm, "a");
        InterfacePtr i1 = im->instantiate(cm, "b");
        InterfacePtr i2 = im->instantiate(cm, "c");
        REQUIRE(cm->get_facts("interfaces").size() == 3);
        ComponentModelPtr cm2 = pr->instantiate<ComponentModel>();
        cm2->set_name("a whole");
        ComponentPtr c = cm->instantiate(cm2, "X");
        REQUIRE(c->get_facts("interfaces").size() == 3);
        REQUIRE(c->get_interface("a")->get_name() == i->get_name());
        REQUIRE(c->get_interface("b")->get_name() == i1->get_name());
        REQUIRE(c->get_interface("c")->get_name() == i2->get_name());
        REQUIRE(c->get_interface("d") == nullptr);
    }

    pr->clear();

    SECTION("find_nonmatching_interfaces")
    {
        InterfaceModelPtr im = pr->instantiate<InterfaceModel>();
        ComponentModelPtr cm = pr->instantiate<ComponentModel>();
        InterfacePtr i = im->instantiate(cm, "a");
        InterfacePtr i1 = im->instantiate(cm, "b");
        InterfacePtr i2 = im->instantiate(cm, "c");
        ComponentModelPtr cm2 = pr->instantiate<ComponentModel>();
        cm2->set_name("a whole");
        ComponentPtr c = cm->instantiate(cm2, "X");
        REQUIRE(c->get_facts("interfaces").size() == 3);
        REQUIRE(c->find_nonmatching_interfaces().size() == 0);
        // Rename an interface of the component model
        i2->set_property("name", "renamed");
        REQUIRE(cm->get_interfaces(i2->get_type(),"c").size() == 0);
        REQUIRE(cm->get_interfaces(i2->get_type(),i2->get_property("name")).size() == 1);
        // Check if this is now a non-matching interface
        auto nonmatching = c->find_nonmatching_interfaces();
        REQUIRE(nonmatching.size() == 1);
        REQUIRE(nonmatching.begin()->first->get_property("name") == "c");
        REQUIRE(nonmatching.begin()->second.size() == 1);
        REQUIRE(nonmatching.begin()->second[0]->get_property("name") == i2->get_property("name"));
    }
}

TEST_CASE("Test ComponentModel class interface", "ComponentModel")
{
    XTypeRegistryPtr pr = std::make_shared<ProjectRegistry>();

    SECTION("composed_of")
    {
        ComponentModelPtr cm = pr->instantiate<ComponentModel>();
        ComponentPtr c = pr->instantiate<Component>();
        cm->composed_of(c);
        REQUIRE(cm->get_facts("parts").size() == 1); // composed_of adds xtype to parts
    }

    pr->clear();

    SECTION("get_part")
    {
        ComponentModelPtr cm = pr->instantiate<ComponentModel>();
        ComponentPtr c1 = pr->instantiate<Component>();
        c1->set_name("A");
        ComponentPtr c2 = pr->instantiate<Component>();
        c2->set_name("B");
        cm->composed_of(c1);
        cm->composed_of(c2);
        REQUIRE(cm->get_facts("parts").size() == 2); // composed_of adds xtype to parts
        REQUIRE(cm->get_part("A")->get_name() == c1->get_name());
        REQUIRE(cm->get_part("B")->get_name() == c2->get_name());
        REQUIRE(cm->get_part("C") == nullptr);
    }

    pr->clear();

    SECTION("instantiate")
    {
        ComponentModelPtr cm = pr->instantiate<ComponentModel>();
        cm->set_all_unknown_facts_empty();
        ComponentModelPtr whole = pr->instantiate<ComponentModel>();
        whole->set_name("WHOLE");
        ComponentPtr ptr = cm->instantiate(whole, "test");
        REQUIRE(ptr->has_property("name"));
        REQUIRE(ptr->get_properties()["name"] == "test");
        REQUIRE(ptr->get_type()->uuid() == cm->uuid());
    }

    pr->clear();

    SECTION("build")
    {
        ComponentModelPtr root_cm = pr->instantiate<ComponentModel>();
        root_cm->set_name("root");
        root_cm->set_all_unknown_facts_empty();
        ComponentModelPtr first_cm = pr->instantiate<ComponentModel>();
        first_cm->set_name("first_cm");
        first_cm->set_all_unknown_facts_empty();
        ComponentModelPtr second_cm = pr->instantiate<ComponentModel>();
        second_cm->set_name("second_cm");
        second_cm->set_all_unknown_facts_empty();
        InterfaceModelPtr some_im = pr->instantiate<InterfaceModel>();
        some_im->instantiate(root_cm, "root_if", "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        some_im->instantiate(first_cm, "first_if", "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        some_im->instantiate(second_cm, "second_if", "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        second_cm->instantiate(first_cm, "part_of_first", true);
        first_cm->instantiate(root_cm, "part_of_root", true);
        // We should now have: root_cm->part_of_root->first_cm->part_of_first->second_cm
        ModulePtr module = root_cm->build("built_root");
        module->set_all_unknown_facts_empty();
        REQUIRE(module->get_facts("interfaces").size() == 1);
        REQUIRE(module->get_facts("parts").size() == 1);
        REQUIRE(module->get_part("part_of_root")->get_facts("parts").size() == 1);
    }

    pr->clear();

    SECTION("has")
    {
        InterfaceModelPtr im = pr->instantiate<InterfaceModel>();
        ComponentModelPtr cm = pr->instantiate<ComponentModel>();
        im->instantiate(cm, "P", "BIDIRECTIONAL", "ONE");
        im->instantiate(cm, "I", "BIDIRECTIONAL", "ONE");
        im->instantiate(cm, "ref", "INCOMING", "ONE");
        im->instantiate(cm, "behavior_state", "INCOMING", "ONE");
        im->instantiate(cm, "axial_2", "OUTGOING", "ONE");
        REQUIRE(cm->get_facts("interfaces").size() == 5);
    }

    pr->clear();

    SECTION("get_interfaces")
    {
        InterfaceModelPtr im = pr->instantiate<InterfaceModel>();
        InterfaceModelPtr im2 = pr->instantiate<InterfaceModel>();
        im2->set_name("Peter Klaus");
        ComponentModelPtr cm = pr->instantiate<ComponentModel>();
        im->instantiate(cm, "a", "BIDIRECTIONAL", "ONE");
        im->instantiate(cm, "b", "BIDIRECTIONAL", "ONE");
        REQUIRE(cm->get_interfaces().size() == 2);
        REQUIRE(cm->get_interfaces(im,"a").size() == 1);
        REQUIRE(cm->get_interfaces(im,"b").size() == 1);
        REQUIRE(cm->get_interfaces(im).size() == 2);
        REQUIRE(cm->get_interfaces(im2).size() == 0);
    }

    pr->clear();

    SECTION("find_nonmatching_part_interfaces")
    {
        InterfaceModelPtr im = pr->instantiate<InterfaceModel>();
        ComponentModelPtr cm = pr->instantiate<ComponentModel>();
        InterfacePtr i = im->instantiate(cm, "a");
        InterfacePtr i1 = im->instantiate(cm, "b");
        InterfacePtr i2 = im->instantiate(cm, "c");
        ComponentModelPtr cm2 = pr->instantiate<ComponentModel>();
        cm2->set_name("a whole");
        ComponentPtr c = cm->instantiate(cm2, "X");
        REQUIRE(c->get_facts("interfaces").size() == 3);
        REQUIRE(cm2->find_nonmatching_part_interfaces().size() == 0);
        // Rename an interface of the component model
        i2->set_property("name", "renamed");
        REQUIRE(cm->get_interfaces(i2->get_type(),"c").size() == 0);
        REQUIRE(cm->get_interfaces(i2->get_type(),i2->get_property("name")).size() == 1);
        // Check if this is now a non-matching interface
        auto nonmatching = cm2->find_nonmatching_part_interfaces();
        REQUIRE(nonmatching.size() == 1);
        REQUIRE(nonmatching.begin()->first->get_property("name") == "c");
        REQUIRE(nonmatching.begin()->second.size() == 1);
        REQUIRE(nonmatching.begin()->second[0]->get_property("name") == i2->get_property("name"));
    }

    pr->clear();

    SECTION("derive_domain_from_parts")
    {
        ComponentModelPtr cm = pr->instantiate<ComponentModel>();
        REQUIRE_THROWS(cm->derive_domain_from_parts());
        ComponentModelPtr domain1 = pr->instantiate<ComponentModel>();
        domain1->set_property("domain", "SOFTWARE");
        domain1->set_all_unknown_facts_empty();
        ComponentPtr c1 = domain1->instantiate(cm, "A");
        REQUIRE(cm->derive_domain_from_parts() == true);
        REQUIRE(cm->get_property("domain") == domain1->get_property("domain"));
        ComponentModelPtr domain2 = pr->instantiate<ComponentModel>();
        domain2->set_property("domain", "MECHANICS");
        domain2->set_all_unknown_facts_empty();
        ComponentPtr c2 = domain2->instantiate(cm, "B");
        REQUIRE(cm->get_facts("parts").size() == 2);
        REQUIRE(cm->derive_domain_from_parts() == true);
        REQUIRE(cm->get_property("domain") != domain1->get_property("domain"));
        REQUIRE(cm->get_property("domain") != domain2->get_property("domain"));
        REQUIRE(cm->get_property("domain") == "ASSEMBLY");
    }

    // TODO: disconnect_parts()
    // TODO: subclass_of()
    // TODO: uri() and uuid()
}

TEST_CASE("Test ComponentModel real-world example", "ComponentModel")
{
    XTypeRegistryPtr pr = std::make_shared<ProjectRegistry>();
    ComponentModelPtr cm = pr->instantiate<ComponentModel>();
    cm->set_properties({
        {"name", "PID"},
        {"domain", "SOFTWARE"},
        {"version", "v0.1"},
        {"defaultConfiguration", {
            {"test_var", "{{ A_GLOBAL_VARIABLE}}"},
            {"faulty_var", "{{ AN_UNKNOWN_VARIABLE}}"},
        }}
    });
    cm->set_all_unknown_facts_empty();
    InterfaceModelPtr im = pr->instantiate<InterfaceModel>();
    im->set_all_unknown_facts_empty();
    im->set_properties({{"name", "float"},
                        {"domain", "SOFTWARE"}});

    im->instantiate(cm, "P", "BIDIRECTIONAL", "ONE", true);
    im->instantiate(cm, "I", "BIDIRECTIONAL", "ONE", true);
    im->instantiate(cm, "D", "BIDIRECTIONAL", "ONE", true);
    im->instantiate(cm, "reference", "INCOMING", "ONE", true);
    im->instantiate(cm, "current", "INCOMING", "ONE", true);
    im->instantiate(cm, "out", "OUTGOING", "N", true);
    REQUIRE(cm->get_facts("interfaces").size() == 6);

    cm->get_facts("interfaces")[0].target.lock()->set_property("alias", "some alias");

    ComponentModelPtr cm2 = pr->instantiate<ComponentModel>();
    cm2->set_properties({
        {"name", "CascadedController"}, {"domain", "SOFTWARE"}, {"version", "v0.1"},
        {"defaultConfiguration", {
            {"a_normal_config_key", "with_a_normal_value"},
            {"another_config_key", "{{ A_GLOBAL_VARIABLE}}"},
            // The following case is not yet supported by inja.
            // see https://github.com/pantor/inja/issues/271
            //{"a_dict_config", "{{ A_DICT_VARIABLE}}"},
            {"a_nested_dict_config", "{{ A_DICT_VARIABLE.SOME_KEY}}"}
        }},
        {"data", {
            {"globalVariables", {
                {"A_GLOBAL_VARIABLE", "a global variable has been filled in"},
                {"A_DICT_VARIABLE", {{"SOME_KEY", "some_value"}, {"ANOTHER_KEY", "another_value"}}}
            }}
        }}
    });
    cm2->set_all_unknown_facts_empty();
    InterfacePtr outerInput = im->instantiate(cm2, "position_reference", "INCOMING", "ONE", true);
    InterfacePtr outerOutput = im->instantiate(cm2, "out", "OUTGOING", "N", true);
    cm->instantiate(cm2, "Velocity", true);
    cm->instantiate(cm2, "Position", true);

    REQUIRE(cm2->get_facts("parts").size() == 2);
    bool found = false;
    InterfacePtr velocity{nullptr};
    InterfacePtr position{nullptr};
    for (const auto &[p, _] : cm2->get_facts("parts"))
    {
        const ComponentPtr co(std::static_pointer_cast<Component>(p.lock()));
        if (!(co->get_name() == "Position" || co->get_name() == "Velocity"))
        {
            continue;
        };
        for (const auto &[s, __] : co->get_facts("interfaces"))
        {
            const InterfacePtr i(std::static_pointer_cast<Interface>(s.lock()));
            // Set some part interface alias to something different
            if ((co->get_name() == "Position") && (i->get_name() == "P"))
            {
                REQUIRE(i->get_alias() == "some alias");
                i->set_alias("modified");
                REQUIRE(i->get_alias() == "modified");
            }
            // Export inner interfaces
            if ((co->get_name() == "Position") && (i->get_name() == "reference"))
            {
                outerInput->alias_of(i);
                REQUIRE(outerInput->get_facts("original").size() == 1);
            }
            if ((co->get_name() == "Velocity") && (i->get_name() == "out"))
            {
                outerOutput->alias_of(i);
                REQUIRE(outerOutput->get_facts("original").size() == 1);
            }
            // Resolve interfaces to be connected
            if ((co->get_name() == "Position") && (i->get_name() == "out"))
            {
                position = i;
            }
            if ((co->get_name() == "Velocity") && (i->get_name() == "reference"))
            {
                velocity = i;
            }
        }
    }
    // make sure we have things to connect
    REQUIRE(velocity);
    REQUIRE(position);
    REQUIRE(position->connected_to(velocity, {{"name", "test"}}) == true);
    int connections = 0;
    for (const auto &[conn, _] : cm2->get_facts("parts"))
    {
        const ComponentPtr s(std::static_pointer_cast<Component>(conn.lock()));
        for (const auto &[inconn, _] : s->get_facts("interfaces"))
        {
            const InterfacePtr si(std::static_pointer_cast<Interface>(inconn.lock()));
            for (const auto &[othersconn, _] : si->get_facts("others"))
            {
                const InterfacePtr ti(std::static_pointer_cast<Interface>(othersconn.lock()));
                connections += 1;
            }
        }
    }
    REQUIRE(connections == 1);

    SECTION("Basic JSON import/export")
    {
        std::map< std::string, nl::json > exported = cm2->export_to();
        XTypePtr imported = XType::import_from(exported.at(cm2->uri()), pr);
        REQUIRE(imported);
        REQUIRE(imported->uri() == cm2->uri());
    }

    SECTION("Build a module")
    {
        ModulePtr module = cm2->build("CascadedControllerModule");
        module->set_all_unknown_facts_empty();
        REQUIRE(module->get_facts("parts").size() == 2);
        std::function<void(const ModulePtr&)> dive_into_module = [&](const ModulePtr& _module){
            for (const auto &[p, _] : _module->get_facts("parts"))
            {
                const ModulePtr part(std::static_pointer_cast<Module>(p.lock()));
                REQUIRE_NOTHROW(part->get_facts("parts"));
                dive_into_module(part);
            }
        };
        dive_into_module(module);
    }

    SECTION("Apply global variables")
    {
        ModulePtr module = cm2->build("CascadedControllerModule");
        module->set_all_unknown_facts_empty();

        REQUIRE(module->get_configuration().at("a_normal_config_key") == "with_a_normal_value");
        REQUIRE(module->get_configuration().at("another_config_key") == "{{ A_GLOBAL_VARIABLE}}");
        REQUIRE_THROWS(module->apply_global_variables());
        nl::json data = cm2->get_data();
        data["globalVariables"]["AN_UNKNOWN_VARIABLE"] = 35;
        cm2->set_data(data);
        REQUIRE_NOTHROW(module->apply_global_variables());
        REQUIRE(module->get_configuration().at("another_config_key") == "a global variable has been filled in");
    }

    SECTION("Basic model import/export")
    {
        std::string exported_json_str = cm2->export_to_basic_model();
        REQUIRE(!exported_json_str.empty());
        nl::json exported_json = parseJson(exported_json_str);
        REQUIRE(!exported_json.empty());
        //std::cout << "============================\n"
        //           << exported_json.dump(4) << std::endl;

        // We have to commit the important things for import_from_basic_model to be found
        REQUIRE(pr->commit(cm, true));
        REQUIRE(pr->commit(im, true));
        // Lets create an empty, fresh registry
        XTypeRegistryPtr import_reg = std::make_shared<ProjectRegistry>();
        auto load_by_uri = [&](const std::string& uri) -> XTypePtr
        {
            INFO("load_by_uri("+uri+")");
            XTypeCPtr lookup = pr->get_by_uri(uri);
            REQUIRE(lookup != nullptr);
            REQUIRE(import_reg->commit(lookup, true));
            return lookup; 
        };
        import_reg->set_load_func(load_by_uri);
        std::vector<ComponentModelPtr> imported = ComponentModel::import_from_basic_model(exported_json_str, import_reg);
        ////std::cout << "imported size: " << imported.size() << std::endl;
        REQUIRE(imported.size() == 1);

        for (ComponentModelPtr &r : imported)
        {
            // //std::cout << "============================\n"
            //           << parseJson(r->export_to_basic_model()).dump(4) << std::endl;
            REQUIRE(r->uuid() == cm2->uuid());
            ////std::cout << r->uuid() << " ==" << cm2->uuid();
        }
    }

    SECTION("Disconnect parts")
    {
        cm2->disconnect_parts();
        connections = 0;
        for (const auto &[conn, _] : cm2->get_facts("parts"))
        {
            const ComponentPtr s(std::static_pointer_cast<Component>(conn.lock()));
            for (const auto &[inconn, _] : s->get_facts("interfaces"))
            {
                const InterfacePtr si(std::static_pointer_cast<Interface>(inconn.lock()));
                for (const auto &[othersconn, _] : si->get_facts("others"))
                {
                    const InterfacePtr ti(std::static_pointer_cast<Interface>(othersconn.lock()));
                    connections += 1;
                }
            }
        }
        REQUIRE(connections == 0);
    }
}

TEST_CASE("Test abstract and concrete component models can_implement", "AbstractComponentModel")
{
    XTypeRegistryPtr pr = std::make_shared<ProjectRegistry>();

    // Abstract Vehicle
    ComponentModelPtr vehicle = pr->instantiate<ComponentModel>();
    vehicle->set_all_unknown_facts_empty();
    InterfaceModelPtr im = pr->instantiate<InterfaceModel>();
    im->set_properties({{"name", "Street"},
                        {"domain", "ASSEMBLY"}});
    InterfaceModelPtr im2 = pr->instantiate<InterfaceModel>();
    im2->set_properties({{"name", "Air"},
                         {"domain", "ASSEMBLY"}});
    vehicle->set_properties({{"name", "Vehicle"},
                             {"domain", "ASSEMBLY"},
                             {"version", "v0.0.1"},
                             {"abstract", true}});
                             
    REQUIRE(vehicle->is_abstract() == true);
    InterfacePtr v_wheel = im->instantiate(vehicle, "wheel", "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
    InterfacePtr v_brake = im->instantiate(vehicle, "brake", "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
    InterfacePtr v_horn = im->instantiate(vehicle, "horn", "OUTGOING", "MULTIPLICITY_NOT_SET", true);
    
    ComponentModelPtr car = pr->instantiate<ComponentModel>();
    car->set_all_unknown_facts_empty();
    car->set_properties({{"name", "Opel"},
                         {"domain", "ASSEMBLY"},
                         {"version", "v0.0.1"},
                          {"abstract", false}});
    InterfacePtr c_wheel = im->instantiate(car, "wheel", "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
    InterfacePtr c_brake = im->instantiate(car, "brake", "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
    InterfacePtr c_steering = im->instantiate(car, "steering", "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
    InterfacePtr c_horn = im->instantiate(car, "horn", "OUTGOING", "MULTIPLICITY_NOT_SET", true);
    InterfacePtr c_headlight = im->instantiate(car, "headlight", "OUTGOING", "MULTIPLICITY_NOT_SET", true);
    InterfacePtr c_audio_system = im->instantiate(car, "audio_system", "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);

    REQUIRE(car->can_implement(vehicle));
}

TEST_CASE("Test abstract and concrete component models", "AbstractComponentModel")
{
    XTypeRegistryPtr pr = std::make_shared<ProjectRegistry>();

    SECTION("implements")
    {
        // Abstract Vehicle
        ComponentModelPtr vehicle = pr->instantiate<ComponentModel>();
        vehicle->set_all_unknown_facts_empty();
        InterfaceModelPtr im = pr->instantiate<InterfaceModel>();
        im->set_properties({{"name", "Street"},
                            {"domain", "ASSEMBLY"}});
        InterfaceModelPtr im2 = pr->instantiate<InterfaceModel>();
        im2->set_properties({{"name", "Air"},
                            {"domain", "ASSEMBLY"}});
        vehicle->set_properties({
            {"name", "Vehicle"},
            {"domain", "ASSEMBLY"},
            {"version", "v0.0.1"},
            {"abstract", true}
        });
        REQUIRE(vehicle->is_abstract() == true);
        InterfacePtr v_wheel = im->instantiate(vehicle, "wheel",  "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        InterfacePtr v_brake = im->instantiate(vehicle, "brake",  "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        InterfacePtr v_horn = im->instantiate(vehicle, "horn", "OUTGOING", "MULTIPLICITY_NOT_SET", true);
        InterfacePtr v_wing = im2->instantiate(vehicle, "wing",  "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        v_wing->set_property("direction", "INCOMING");

        ComponentModelPtr car = pr->instantiate<ComponentModel>();
        car->set_all_unknown_facts_empty();
        car->set_properties({
            {"name", "Opel"},
            {"domain", "ASSEMBLY"},
            {"version", "v0.0.1"},
            {"abstract", false}
        });
        InterfacePtr c_wheel = im->instantiate(car, "wheel",  "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        InterfacePtr c_brake = im->instantiate(car, "brake",  "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        InterfacePtr c_steering = im->instantiate(car, "steering",  "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        InterfacePtr c_horn = im->instantiate(car, "horn", "OUTGOING", "MULTIPLICITY_NOT_SET", true);
        InterfacePtr c_headlight = im->instantiate(car, "headlight", "OUTGOING", "MULTIPLICITY_NOT_SET", true);
        InterfacePtr c_audio_system = im->instantiate(car, "audio_system", "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);

        // Make sure car inherited all needed interfaces of vehicle
        REQUIRE(car->get_interfaces().size() >= vehicle->get_interfaces().size());

        // Fails because we do not match all abstract interfaces
        REQUIRE(car->can_implement(vehicle) == false);
        // Fails because we have not yet mapped any interfaces
        REQUIRE(car->is_valid_implementation(vehicle) == false);
        c_wheel->realizes(v_wheel);
        c_brake->realizes(v_brake);
        // Throws because they are not of the same type
        REQUIRE_THROWS(c_steering->realizes(v_wing));

        InterfacePtr c_wing = im2->instantiate(car, "wing",  "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        c_wing->set_property("direction", "OUTGOING");
        // Throws because the interfaces don't have the same direction
        REQUIRE_THROWS(c_wing->realizes(v_wing));
        c_wing->set_property("direction", "INCOMING");
        REQUIRE(car->can_implement(vehicle) == true);
        c_wing->realizes(v_wing);
        // check has_realization 
        REQUIRE(c_wheel->has_realization(v_wheel) == true);
        c_wheel->unrealize();
        REQUIRE(c_wheel->has_realization(v_wheel) == false);
        //again realize after unrealize
        c_wheel->realizes(v_wheel);
        // map the last remaining interface
        c_horn->realizes(v_horn);
        // Now  it should check the validity
        REQUIRE(car->is_valid_implementation(vehicle) == true);

        // perform implementation
        car->implements(vehicle);
        REQUIRE(car->is_implementing(vehicle) == true);

        // we should only implement vehicle once 
        REQUIRE_THROWS(car->implements(vehicle));
        // Check if we implemented an abstract
        REQUIRE(car->get_abstracts().size() == 1);
        REQUIRE(vehicle->get_implementations().size() == 1);

        // Build the car
        SECTION("Build car")
        {
            ModulePtr real_car = car->build("real_car");
            REQUIRE(real_car->get_facts("interfaces").size() == car->get_interfaces().size());
            REQUIRE(real_car->is_atomic());
        }
        // Build the vehicle (which becomes a car)
        SECTION("Build abstract vehicle")
        {
            ModulePtr real_car = vehicle->build("real_car");
            REQUIRE(real_car->get_facts("interfaces").size() == car->get_interfaces().size());
        }
        // Make another implementation of vehicle
        ComponentModelPtr another_car = pr->instantiate<ComponentModel>();
        another_car->set_all_unknown_facts_empty();
        another_car->set_properties({
            {"name", "VW T4"},
            {"domain", "ASSEMBLY"},
            {"version", "v0.0.1"},
            {"abstract", false}
        });
        InterfacePtr another_c_wheel = im->instantiate(another_car, "wheel",  "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        InterfacePtr another_c_brake = im->instantiate(another_car, "brake",  "DIRECTION_NOT_SET", "MULTIPLICITY_NOT_SET", true);
        InterfacePtr another_c_horn = im->instantiate(another_car, "horn", "OUTGOING", "MULTIPLICITY_NOT_SET", true);
        InterfacePtr another_c_wing = im2->instantiate(another_car, "wing",  "INCOMING", "MULTIPLICITY_NOT_SET", true);
        another_c_wheel->realizes(v_wheel);
        another_c_brake->realizes(v_brake);
        another_c_horn->realizes(v_horn);
        another_c_wing->realizes(v_wing);
        REQUIRE(another_car->can_implement(vehicle));
        another_car->implements(vehicle);
        REQUIRE(another_car->is_implementing(vehicle) == true);
        REQUIRE(car->get_abstracts().size() == 1);
        REQUIRE(another_car->get_abstracts().size() == 1);
        REQUIRE(vehicle->get_implementations().size() == 2);

        // Build the vehicle and select one of the implementations using callback
        SECTION("Build abstract vehicle with multiple implementations")
        {
            REQUIRE_THROWS(vehicle->build("real_car"));
            auto select_first_implementation = [](const ComponentModelPtr& abstract_model, const std::vector< ComponentModelPtr >& implementations) -> ComponentModelPtr
            {
                return implementations[0];
            };
            ModulePtr real_car = vehicle->build("real_car", select_first_implementation);
            REQUIRE(real_car->get_facts("interfaces").size() == car->get_interfaces().size());
        }

        // In an upper component model, instantiate vehicle and try to build the upper model using callback to resolve the implementations
        // Create a garage
        ComponentModelPtr garage = pr->instantiate<ComponentModel>();
        garage->set_all_unknown_facts_empty();
        garage->set_properties({
                    {"name", "A Garage"},
                    {"domain", "ASSEMBLY"},
                    {"version", "v0.0.1"},
                    {"abstract", false},
                    {"can_have_parts", true}
                });
        // and place an abstract vehicle inside
        ComponentPtr vehicle_part = vehicle->instantiate(garage, "some car", true);
        SECTION("Build a garage with an abstract vehicle inside")
        {
            REQUIRE_THROWS(garage->build("My garage"));
            auto select_vw_implementation = [&](const ComponentModelPtr& abstract_model, const std::vector< ComponentModelPtr >& implementations) -> ComponentModelPtr
            {
                for (const auto& impl : implementations)
                {
                    if (impl->get_name() == another_car->get_name())
                        return impl;
                }
                return nullptr;
            };
            ModulePtr my_garage = garage->build("My garage", select_vw_implementation);
            REQUIRE(my_garage->get_facts("parts").size() == 1);
            REQUIRE(my_garage->get_part("some car")->get_name() == vehicle_part->get_name());
            REQUIRE(!my_garage->is_atomic());
        }

        // Export some of the interfaces to the garage interfaces, create a park of garages, connect some of the interfaces, then build that park
        // Create a park
        ComponentModelPtr park = pr->instantiate<ComponentModel>();
        park->set_all_unknown_facts_empty();
        park->set_properties({
                    {"name", "A park"},
                    {"domain", "ASSEMBLY"},
                    {"version", "v0.0.1"},
                    {"abstract", false},
                    {"can_have_parts", true}
                });
        // Export an interface of the abstract vehicle inside the garage (makes no sense but hey :)
        InterfacePtr g_horn = im->instantiate(garage, "inner horn", "OUTGOING", "MULTIPLICITY_NOT_SET", true);
        g_horn->alias_of(vehicle_part->get_interface(v_horn->get_name()));
        REQUIRE(g_horn->get_facts("original").size() == 1);
        // Lets create a detector for that horn signal (to which we can connect)
        ComponentModelPtr listener = pr->instantiate<ComponentModel>();
        listener->set_all_unknown_facts_empty();
        listener->set_properties({
                    {"name", "A listener"},
                    {"domain", "ASSEMBLY"},
                    {"version", "v0.0.1"},
                    {"abstract", false},
                    {"can_have_parts", false}
                });
        im->instantiate(listener, "ear", "INCOMING", "MULTIPLICITY_NOT_SET", true);
        // Add the listener to the park
        listener->instantiate(park, "Henning", true);
        // Add two garages
        garage->instantiate(park, "My garage", true);
        garage->instantiate(park, "Other garage", true);
        REQUIRE(park->get_part("Henning") != nullptr); 
        REQUIRE(park->get_part("My garage") != nullptr); 
        REQUIRE(park->get_part("Other garage") != nullptr); 
        // Connect the horns of the garage to the ear of the listener
        park->get_part("My garage")->get_interface("inner horn")->connected_to(park->get_part("Henning")->get_interface("ear"));
        park->get_part("Other garage")->get_interface("inner horn")->connected_to(park->get_part("Henning")->get_interface("ear"));
        REQUIRE(park->get_part("My garage")->get_interface("inner horn")->get_facts("others").size() == 1);
        REQUIRE(park->get_part("Other garage")->get_interface("inner horn")->get_facts("others").size() == 1);
        REQUIRE(park->get_part("Henning")->get_interface("ear")->get_facts("from_others").size() == 2);

        SECTION("Build a park with two garages with inner abstract vehicles and a listener to their horns")
        {
            REQUIRE_THROWS(park->build("The park"));
            auto select_implementation = [](const ComponentModelPtr& abstract_model, const std::vector< ComponentModelPtr >& implementations) -> ComponentModelPtr
            {
                static int index = 0;
                auto result = implementations[index];
                index = (index + 1) % implementations.size();
                return result;
            };
            ModulePtr the_park = park->build("The park", select_implementation);
            // Test all the alias and connection relations/facts
            REQUIRE(the_park->get_facts("parts").size() == park->get_facts("parts").size());
            REQUIRE(the_park->get_part("My garage")->get_interface("inner horn")->get_facts("others").size() == 1);
            REQUIRE(the_park->get_part("Other garage")->get_interface("inner horn")->get_facts("others").size() == 1);
            REQUIRE(the_park->get_part("Henning")->get_interface("ear")->get_facts("from_others").size() == 2);
            REQUIRE(the_park->get_part("My garage")->get_interface("inner horn")->get_facts("original").size() > 0);
            REQUIRE(the_park->get_part("Other garage")->get_interface("inner horn")->get_facts("original").size() > 0);
            REQUIRE(the_park->get_part("My garage")->get_interface("inner horn")->get_facts("original")[0].target.lock()->uri() == the_park->get_part("My garage")->get_part("some car")->get_interface("horn")->uri());
        }
    }

    pr->clear();

    SECTION("atomic")
    {
        ComponentModelPtr vehicle = pr->instantiate<ComponentModel>();
        vehicle->set_all_unknown_facts_empty();
        ComponentPtr steering = pr->instantiate<Component>();
        vehicle->set_properties({
            {"name", "Vehicle"},
            {"domain", "ASSEMBLY"},
            {"version", "v0.0.1"},
            {"can_have_parts", false}
        });
        REQUIRE_THROWS(vehicle->add_parts(steering));
        vehicle->set_property("can_have_parts", true);
        REQUIRE(vehicle->is_atomic());
        vehicle->add_parts(steering);
        REQUIRE(vehicle->is_atomic() == false);
        vehicle->set_property("can_have_parts", false);
        REQUIRE_THROWS(vehicle->is_atomic(true));

    }
}

TEST_CASE("Test ExternalReference class interface", "ExternalReference")
{
    XTypeRegistryPtr pr = std::make_shared<ProjectRegistry>();

    SECTION("add reference")
    {
        ComponentModelPtr cm = pr->instantiate<ComponentModel>();
        GitReferencePtr ref = pr->instantiate<GitReference>();
        ref->set_property("remote_url", "https://github.com/dfki-ric/dfki-ric.github.io.git");
        ref->set_property("name", "test_repo");
        cm->set_name("SOMETHING");
        cm->annotate_with(ref, (bool)false);
        REQUIRE(cm->get_facts("external_references").size() == 1);
        REQUIRE(std::dynamic_pointer_cast<ExternalReference>(cm->get_facts("external_references").at(0).target.lock())->get_name() == "test_repo");
    }

    pr->clear();

    SECTION("checkout reference")
    {
        static std::once_flag onceFlag;
        std::call_once(onceFlag, [&]{              
        GitReferencePtr ref = pr->instantiate<GitReference>();
        std::string remote_url = "https://github.com/Priyanka328/dummy_test_libgit2.git";
        ref->set_property("remote_url", remote_url);
        //ref->set_property("remote", nl::json{{"name","origin"},{"url",remote_url}});
        ref->set_property("name", "test_repo");
        nl::json out = ref->load("temp");
        REQUIRE(out["url"] == remote_url);
        REQUIRE(out["local_path"] == "temp/"+std::to_string(ref->uuid()));
        for(int i = 0; i<3; i ++)
        {
            std::string filename = "test_cpp" + std::to_string(i) +".cpp";
            std::ofstream ofs(fs::path("temp") /std::to_string(ref->uuid()) / filename);
            ofs << i;
            ofs.close();
        }
        ref->set_read_only(false);
        //ref->store("temp"); 
        });
    }
}

TEST_CASE("Test git wrapper create/open/ add/initial_commit ", "gitwrapper_test1")
{
   static std::once_flag onceFlag;
    std::call_once ( onceFlag, [ ]{

        fs::remove_all("my_repo");
        Repository repo;
        repo.create("my_repo"); // SOFTWARE/PID/v0.01
        repo.open("my_repo");
        std::ofstream ofs( repo.get_repository_directory() / "README.md" );
        ofs <<  "this repo is to test the libgit2 library ";
        ofs.close();
        repo.add("README.md");
        repo.commit("initial commit" );


     std::cout << "Testcase: " << 1 << std::endl;
     });


}

TEST_CASE("Test git wrapper add_remote /push ", "gitwrapper_test2")
{
   static std::once_flag onceFlag;
    std::call_once ( onceFlag, [ ]{

        Repository repo;
        repo.open("my_repo");
        repo.add_remote("origin", "https://github.com/dfki-ric/dfki-ric.github.io.git");
        //repo.push();


     std::cout << "Testcase: " << 2 << std::endl;
     });

}

TEST_CASE("Test git wrapper add all / commit/ push initial commit", "gitwrapper_test3")
{
   static std::once_flag onceFlag;
    std::call_once ( onceFlag, [ ]{

        Repository repo;
        repo.open("my_repo");

        auto path = repo.get_repository_directory();
        for(int i = 0; i<3; i ++)
        {
            std::string filename = "test_" + std::to_string(i) +".cpp";
            std::ofstream ofs(path / filename);
            ofs << i;
            ofs.close();
        }
        repo.add_all();
        repo.commit("added new implementation" );
        //repo.push();

             std::cout << "Testcase: " << 3 << std::endl;

     });


}



TEST_CASE("Test git wrapper clone", "gitwrapper_test4")
{
   static std::once_flag onceFlag;

    std::call_once ( onceFlag, [ ]{
        fs::remove_all("cloned_repo");
        fs::create_directory("cloned_repo");
        Repository::clone("https://github.com/dfki-ric/dfki-ric.github.io.git","cloned_repo");
        Repository repo;
        repo.open("cloned_repo");
        REQUIRE(repo.current_branch_name() == "master");
        repo.create_branch("test");
        repo.checkout("test");
        REQUIRE(repo.current_branch_name() == "test");
        auto path = repo.get_repository_directory();
        for(int i = 0; i<3; i ++)
        {
            std::string filename = "clone_test_" + std::to_string(i) +".cpp";
            std::ofstream ofs(path / filename);
            ofs << i;
            ofs.close();
        }

        repo.add_all();
        // The following will fail due to permissions
        //repo.push("origin","test");

             std::cout << "Testcase: " << 4 << std::endl;

     });
}




// TEST_CASE("Test git wrapper pull", "gitwrapper_test5")
// {
//   static std::once_flag onceFlag;

//    std::call_once ( onceFlag, [ ]{

//        fs::create_directory("cloned_repo5");
//        Repository::clone("git@git.hb.dfki.de:pchowdhury/libgit2_wrapper_test.git","cloned_repo5");
//        Repository repo;
//        repo.open("cloned_repo5");
//        repo.create_branch("test");
//        repo.checkout("test");
//        repo.pull("origin","test");
//        //repo.pull("origin","master");
//        //repo.push("origin","test");

//        //std::cout << "Testcase: " << 5 << std::endl;

//     });

// }

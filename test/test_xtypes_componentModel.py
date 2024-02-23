import unittest
from xtypes_generator_py import XType
from xtypes_py import ComponentModel, InterfaceModel, Component, ProjectRegistry


class TestComponentModel(unittest.TestCase):
    def test_component_model(self):
        print("Test component model")
        registry = ProjectRegistry()
        cm = ComponentModel()
        cm.set_all_unknown_facts_empty()
        cm2 = ComponentModel()
        cm2.set_all_unknown_facts_empty()
        print(cm.get_properties())
        print(cm.get_relations())
        im = InterfaceModel()
        im.set_all_unknown_facts_empty()

        print("Setup a real world example")
        im.set_properties({"name": "float", "domain": "SOFTWARE"})
        cm.set_properties({"name": "PID", "domain": "SOFTWARE", "version": "v0.1"})
        registry.commit(im, True)
        im.instantiate(cm, "P", "BIDIRECTIONAL", "ONE", True)
        im.instantiate(cm, "I", "BIDIRECTIONAL", "ONE", True)
        im.instantiate(cm, "D", "BIDIRECTIONAL", "ONE", True)
        im.instantiate(cm, "reference", "INCOMING", "ONE", True)
        im.instantiate(cm, "current", "INCOMING", "ONE", True)
        im.instantiate(cm, "out", "OUTGOING", "N", True)
        assert len(cm.get_facts("interfaces")) == 6

        print("Create a cascaded controller")
        cm2.set_properties({"name": "CascadedController", "domain": "SOFTWARE", "version": "v0.1"})
        registry.commit(cm, True)
        cm.instantiate(cm2, "Velocity", True)
        cm.instantiate(cm2, "Position", True)
        source = None
        for p, _ in cm2.get_facts("parts"):
            if p.name != "Position":
                continue
            for s, _ in p.get_facts("interfaces"):
                if s.name == "out":
                    source = s
                    break
        assert source is not None
        target = None
        for v, _ in cm2.get_facts("parts"):
            if v.name != "Velocity":
                continue
            for t, _ in v.get_facts("interfaces"):
                if t.name == "reference":
                    target = t
                    break
        assert target is not None
        source.connected_to(target, {'name': 'test'})
        assert len(source.get_facts("others")) == 1
        assert len(target.get_facts("from_others")) == 1
        assert len(source.get_facts("parent")) == 1
        assert len(target.get_facts("parent")) == 1
        assert source.get_facts("parent") != target.get_facts("parent")
        registry.commit(cm2, True)

        print("Test JSON export/import I")
        exported = cm2.export_to()

        for uri, spec in exported.items():
            print(spec)

        new_registry = ProjectRegistry()
        def load_by_uri(uri) -> XType:
            print(f"load_by_uri({uri})")
            imp = registry.get_by_uri(uri)
            assert imp
            assert new_registry.commit(imp, True) == True
            return new_registry.get_by_uri(uri)
        new_registry.set_load_func(load_by_uri)

        cm2_imp = XType.import_from(exported[cm2.uri()], new_registry)
        print(cm2_imp)
        assert cm2_imp.uri() == cm2.uri()
        assert type(cm2_imp) == ComponentModel
        assert len(cm2_imp.get_facts("parts")) > 0
        assert len(cm2.get_facts("parts")) > 0
        assert len(cm2_imp.get_facts("parts")) == len(cm2.get_facts("parts"))
        for rp, _ in cm2_imp.get_facts("parts"):
            found = False
            for p, _ in cm2.get_facts("parts"):
                if p.name == rp.name:
                    found = True
            assert found is True

        print("Test JSON export/import II")
        exported = cm2.export_to_basic_model()
        print(exported)

        new_registry = ProjectRegistry()
        def load_by_uri(uri) -> XType:
            print(f"load_by_uri({uri})")
            imp = registry.get_by_uri(uri)
            assert imp
            assert new_registry.commit(imp, True) == True
            return new_registry.get_by_uri(uri)
        new_registry.set_load_func(load_by_uri)

        imported = ComponentModel.import_from_basic_model(exported, new_registry)
        assert len(imported) == 1
        for r in imported:
            assert type(r) == ComponentModel
            assert r.uuid() == cm2.uuid()
            for rp, _ in r.get_facts("parts"):
                found = False
                for p, _ in cm2.get_facts("parts"):
                    if p.name == rp.name:
                        found = True
                assert found is True

        print("Test disconnect")
        connections = 0
        for s, _ in cm2.get_facts("parts"):
            print(s.name)
            # FIXME: This fails somehow with "No registry". But it worked before!!!
            # Are the parts committed?
            print(registry.knows_uri(s.uri()))
            for si, _ in s.get_facts("interfaces"):
                for ti, _ in si.get_facts("others"):
                    connections += 1
        assert connections == 1
        cm2.disconnect_parts()
        connections = 0
        for s, _ in cm2.get_facts("parts"):
            for si, _ in s.get_facts("interfaces"):
                for ti, _ in si.get_facts("others"):
                    connections += 1
        assert connections == 0

    def test_composed_of(self):
        cm = ComponentModel()
        c = Component()
        cm.composed_of(c)
        assert len(cm.get_facts("parts")) == 1

    def test_has(self):
        registry = ProjectRegistry()
        cm = ComponentModel()
        im = InterfaceModel()
        registry.commit(im, True)
        im.instantiate(cm, "P", "BIDIRECTIONAL", "ONE")
        im.instantiate(cm, "I", "BIDIRECTIONAL", "ONE")
        im.instantiate(cm, "ref", "INCOMING", "ONE")
        im.instantiate(cm, "behavior_state", "INCOMING", "ONE")
        im.instantiate(cm, "axial_2", "OUTGOING", "ONE")
        assert len(cm.get_facts("interfaces")) == 5

    def test_instantiate(self):
        registry = ProjectRegistry()
        cm = ComponentModel()
        cm.set_all_unknown_facts_empty()
        whole = ComponentModel()
        whole.set_all_unknown_facts_empty()
        whole.name = "whole"
        registry.commit(cm, True)
        test = cm.instantiate(whole, "test", True)
        assert test.has_property("name")
        assert test.get_properties()["name"] == "test"



if __name__ == '__main__':
    unittest.main()

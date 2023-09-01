import unittest
import xtypes_py 

class TestComponent(unittest.TestCase):

    def test_instance_of(self):
        cm = xtypes_py.ComponentModel()
        c = xtypes_py.Component()
        model = xtypes_py.ComponentModel()
        c.add_model(model)
        assert c.has_relation("model")
        assert c.has_facts("model")
        assert c.get_facts("model")
        assert len(c.get_facts("model")) > 0 
        assert type(c.get_facts("model")[0][0]) == xtypes_py.ComponentModel   
    
    def test_part_of(self):
        cm = xtypes_py.ComponentModel()
        cm.set_all_unknown_facts_empty()
        c = xtypes_py.Component()
        c.set_all_unknown_facts_empty()
        whole = xtypes_py.ComponentModel()
        whole.set_all_unknown_facts_empty()
        c.add_whole(whole)
        assert c.has_relation("model")
        assert c.has_facts("model")
        assert c.get_facts("whole")
        assert len(c.get_facts("whole")) == 1 
        assert type(c.get_facts("whole")[0][0]) == xtypes_py.ComponentModel

    def test_has(self):
        c = xtypes_py.Component()
        i= xtypes_py.Interface()
        c.add_interfaces(i)
        assert c.get_interface("UNKNOWN")
    
    def test_get_interface(self):
        c = xtypes_py.Component()
        i = xtypes_py.Interface()
        local_interface_instance = xtypes_py.Interface()
        i.name = "name1"
        c.add_interfaces(local_interface_instance)
        local_interface_instance.name = "name2"
        c.add_interfaces(i)
        obj1 = c.get_interface("name1")
        obj2 = c.get_interface("name2")
        assert obj1.name == i.name
        assert obj1 == i
        assert obj2.name == local_interface_instance.name


if __name__ == '__main__':

    unittest.main()

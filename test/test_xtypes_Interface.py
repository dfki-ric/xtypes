import unittest
import xtypes_py 


class TestInterface(unittest.TestCase):

    def test_instance_of(self):
        i= xtypes_py.Interface()
        model = xtypes_py.InterfaceModel()
        i.add_model(model)
        assert i.has_facts("model")
        assert len(i.get_facts("model")) == 1 
        assert type(i.get_facts("model")[0][0]) == xtypes_py.InterfaceModel

    def test_alias_of(self):
        interface = xtypes_py.Interface()
        interface2 = xtypes_py.Interface()
        interface.add_original(interface2)
        assert len(interface.get_facts("original")) == 1

    def test_child_of(self):
        i = xtypes_py.Interface()
        cm = xtypes_py.ComponentModel()
        i.child_of(cm)
        assert len(i.get_facts("parent")) == 1
        
    def test_connected_to(self):
        pass
        # FIXME: This is a bad test
        #i = xtypes_py.Interface()
        #i1 = xtypes_py.Interface()
        #i2 = xtypes_py.Interface()
        #i.connected_to(i1, {})
        #i.connected_to(i2, {})
        #assert len(i.get_facts("others")) == 2
    

if __name__ == '__main__':

    unittest.main()

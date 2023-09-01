import unittest
import xtypes_py 


class TestInterfaceModel(unittest.TestCase): 
    def test_instantiate(self):
        registry = xtypes_py.ProjectRegistry()
        im = xtypes_py.InterfaceModel()
        registry.commit(im, True)
        whole = xtypes_py.ComponentModel()
        registry.commit(whole, True)
        test = im.instantiate(whole, "test")
        assert test.has_property("name")
        assert test.get_properties()["name"] == "test"

    def test_subclass_of(self):
        im = xtypes_py.InterfaceModel()
        model = xtypes_py.InterfaceModel()
        im.add_model(model)
        assert im.has_facts("model")
        assert len(im.get_facts("model")) == 1

if __name__ == '__main__':

    unittest.main()

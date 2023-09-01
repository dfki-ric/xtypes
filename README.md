# X-Types
This library defines the main entities & relations of the X-Rock project(s).
XTypes are a generic type of objects. The speciality of this type is that it has a standardized type definition that allows to add properties and relations to other XTypes.

X-Types was initiated and is currently developed at the
[Robotics Innovation Center](http://robotik.dfki-bremen.de/en/startpage.html) of the
[German Research Center for Artificial Intelligence (DFKI)](http://www.dfki.de) in Bremen,
together with the [Robotics Group](http://www.informatik.uni-bremen.de/robotik/index_en.php)
of the [University of Bremen](http://www.uni-bremen.de/en.html).


## Motivation & Understanding the Concept
To represent any hardware and software component of modular robots we developed X-Types. They provide an easy to use representation, with C++ and python interfaces.
X-Types can be stored, read and maintained in a database see (the [xdbi](https://github.com/dfki-ric/xdbi) package).

A *ComponentModel* describes a type of entity like a blueprint, a *Component* is then it's instantiation.
The same principal applies for *InterfaceModels* and *Interfaces*, they define connections between *Components*.

A connected set of *Components* can be stored and reused as new *ComponentModel*.

*Modules* are unique *Components*.

X-Types also provides *ExternalReferences* which offers the possibility to annotate ComponentModels with links to other external data sources.

**Example**: You have a robotic arm with three revolute joints. All joints use the same motor.
In the database you already have defined a *ComponentModel* which represents it in general.
Also you have defined an *InterfaceModel* which represents the flange of the motor and specified that the motor has two *Interfaces* of this *InterfaceModel*
The same you have done for the links in between.
Now we can instantiate the motor-*ComponentModel* to three motor-*Components* and the links as well.
Each motor-*Component* is annotated with the necessary information e.g. an ID or a zero-pose.
Via their *Interfaces* we now specify how everything is connected.

This new setup can now be stored as *ComponentModel* and be used in bigger assemblies.

As soon as we really build this robot. Each *Component* has to be realized by a *Module*.
The *Module* has the same properties as the *Component* but relates to one single entity.

If you have two of those arms, both have the same *Components*, but not the same *Modules* as e.g. each motor has it's own controller with it's own MAC-adress.

X-Types make use of the [X-Types-Generator](https://github.com/dfki-ric/xtypes_generator) and are thereby compatible with all other types generated using the xtypes_generator.

## Getting Started

You can either use the python or the C++ interface to use and maintain your X-Types.
In the following you find a basic python example.

```python
import xtypes_py

cm = xtypes_py.ComponentModel()
cm.name = "Motor"
cm.domain = "HARDWARE"
...

im = xtypes_py.InterfaceModel()
im.name = "Flange"
im.domain = "HARDWARE"

# now we state that our motor has two flanges
im.instantiate(cm, "base")
im.instantiate(cm, "drive_side")
```

The usage in C++ works analogous.
Just use `#include <xtypes/xtypes.hpp>` to have everything available.

For direct usage without implementation consider using the [X-Rock-GUI](https://github.com/dfki-ric/xrock_gui_model).

## Requirements
- CMake >=3.10
- [X-Types-Generator](https://github.com/dfki-ric/xtypes_generator)
- libgit2
- python3
- [XDBI](https://github.com/dfki-ric/xdbi)

>NOTE: The scripts that are part of XTypes assume a UNIX based system

## Installation
Either via CMake:
```bash
mkdir build
cd build
cmake ..
make install
```

As soon as this package is integrated into one of our package_sets, you can of course also use autoproj's amake

## Documentation
After building you can generate the API documentation using `make doc` in the build directory.

The API documentation is generated into build/doc/html.

## Testing
```bash
cd build
make cpp_test # for testing the C++ interface
make py_test # for testing the python interface
```

### Entities
You'll find further information on the here defined types in [doc/Entities.md](doc/Entities.md)

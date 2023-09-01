## Entities

### D-Rock

The following entities are based on the models developed in the D-Rock project

#### ComponentModel

The *ComponentModel* is the main entity from the D-Rock project.
It defines the interfaces, properties and inner structure of a resusable component from the

* SOFTWARE
* MECHANICS
* ELECTRONICS
* COMPUTATION
* BEHAVIOR
* ASSEMBLY

domains.
The *ASSEMBLY* domain is somewhat special as it is the only domain in which component models can be composed of parts of the other domains.

##### Subtypes

The component model has a *type* field which can hold an arbitrary string.
This type field represents an implicit *SUBCLASS_OF* relationship (see below).
In the database it is encoded as an edge between the subtype and its supertype.
This type information is used to link into an ontological knowledge base.
In the following the URI/IRI links in the database are listed:


| Domain | Short Type | URI/IRI | Comments |
|--------|------------|---------|----------|
| MECHANICS | Link | | Represents a mechanical link (see URDF) |
| MECHANICS/ASSEMBLY | Joint | | Represents a mechanical/logical joint (see URDF) |
| MECHANICS | Gear | | Represents a gear of a motor |
| MECHANICS/ELECTRONICS/ASSEMBLY | BLDC Rotor Stator | | Represents either the mechanical/electronical or composite model of the rotor/stator pair of a motor |
| MECHANICS | BLDC Housing | | Represents the housing of a motor |
| Any | UNKNOWN | | Representss the root of all things (default type)|
| COMPUTATION/ELECTRONICS/ASSEMBLY | Communication Device | | Represents a component for communication |
| COMPUTATION/ELECTRONICS/ASSEMBLY | Processing Device | | Represents a component for executing software |
| COMPUTATION | Conventional | | Represents a **conventional** processing device (CPU) |
| COMPUTATION | FPGA | | Represents configurable hardware for processing (FPGA) |
| | Joint Controller | | Represents a piece of software controlling a joint |
| ELECTRONICS | PCB | | Represents a printed circuit board |
| | structure | | |
| | gripper | | |
| | arm_gripper | | |
| | control | | |
| | adapter | | |
| SOFTWARE | system_modelling::task_graph::Task | | Represents a ROCK task. Should be renamed |

#### Component

The *Component* is always an instance of a *ComponentModel*.
It defines a concrete individual of that model - mostly as a part of a composite component model.

#### InterfaceModel

The *InterfaceModel* is an extension to the D-Rock definitions.
It is the class from which conrete interface instances of an component are derived.
An example would be a **Rock Input Port**.

#### Interface

The *Interface* is always an instance of an *InterfaceModel*.
Normally, an interface is bound to a component or component model to sepcify the interaction points or boundary of that entity.

#### Relations
You can find further information on the Relations in xtypes_generator/doc/Relations.md

In the following we link the relations/edges of our database to possible ontological knowledge bases.
Some of these relations might need multiple URI/IRI dependent on their domain and range.

| Relation | URI/IRI | Comments |
| -------- | ------- | -------- |
| HAS      |         | Component(Model) HAS Interface |
| PART_OF_COMPOSITION | | Component PART_OF_COMPOSITION ComponentModel |
| SUBCLASS_OF |      | ComponentModel SUBCLASS_OF ComponentModel |
| INSTANCE_OF |      | Component INSTANCE_OF ComponentModel |
| CONNECTED_TO |     | Interface CONNECTED_TO Interface |
| ALIAS_OF |         | Interface ALIAS_OF Interface |
| NEEDS | | |
| PROVIDES | | |
| CONTAINED_BY | | |
| EXISTS_IN | | |
| GENERATED | | |
| DEPENDS_ON | | |
| CONSTRAINED_BY | | |
| INTERFACE_TO | | |
| SPANS | | |
| HAS_UNIQUE | | |
| ANNOTATES | | |
| ATTACHED_TO | | |

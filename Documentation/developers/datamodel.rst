Writing a YANG data model
^^^^^^^^^^^^^^^^^^^^^^^^^

The service structure is described using a YANG data model.
All the services intended to work with polycube must derive from a base service definition that defines some common structures across all services (e.g., *ports*).

Polcyube includes three different base datamodels:

- :scm_web:`polycube-base <src/services/datamodel-common/polycube-base.yang>`: common definitions for all cube types
- :scm_web:`polycube-standard-base <src/services/datamodel-common/polycube-standard-base.yang>`: base datamodel for standard cubes
- :scm_web:`polycube-transparent-base <src/services/datamodel-common/polycube-transparent-base.yang>`: base datamodel for transparent cubes

The ``polycube-base`` datamodel must be always present, and ``polycube-standard-base`` or ``polycube-trasparent-base`` depending on the type of cube you are developing.

In order to derive from such base datamodel include the following line:

::

  import polycube-base { prefix "polycube-base"; }
  import polycube-standard-base { prefix "polycube-standard-base"; }
  // or
  import polycube-base { prefix "polycube-base"; }
  import polycube-transparent-base { prefix "polycube-transparent-base"; }

In addition, if you are implementing a standard cube and the base definition of a port is not enough for the service, it is possible to use the `augment` keyword to insert additional nodes, as shown below:

::

  uses "polycube-standard-base:standard-base-yang-module" {
    augment ports {
    // Put here additional port's fields.
    }
  }

The easiest way for starting to write a datamodel is to take a look to the existing ones:

- :scm_web:`helloworld.yang <src/services/pcn-helloworld/datamodel/helloworld.yang>`
- :scm_web:`simpleforwarder.yang <src/services/pcn-simpleforwarder/datamodel/simpleforwarder.yang>`
- :scm_web:`simplebridge.yang <src/services/pcn-simplebridge/datamodel/simplebridge.yang>`

Documentation about YANG can be found on `RFC6020 <https://tools.ietf.org/html/rfc6020>`_ and `RFC7950 <https://tools.ietf.org/html/rfc7950>`_.
Polycube Developers Guide
=========================

This guide represents an initial starting point for developers that want to implement new services (e.g., custom NAT, router, etc) using the Polycube software infrastructure.


.. toctree::
   :maxdepth: 2
   :caption: Contents:

   datapath
   controlplane
   datamodel
   codegen
   debugging
   profiler
   hints



How to create a new service / update an existing one
----------------------------------------------------

The process to create or update service could be summarized in these steps:

  1. :doc:`Write or update a datamodel for the service <datamodel>`
  2. :doc:`Use the datamodel for generating a service stub or updating an existing service stub <codegen>`
  3. :doc:`Implement or update the eBPF datapath <datapath>`
  4. :doc:`Implement or update the control plane <controlplane>`

Please note that steps (1) and (2) are needed only when there is a change to the the YANG data model.
In case we have to slightly modify an existing service (e.g., fixing a bug in the code), only steps (3) and (4) are required.


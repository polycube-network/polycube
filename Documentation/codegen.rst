Automatic code generation tools installation guide
==================================================

Polycube automatic code generation tools
This folder contains the scripts and the instructions necessary to install the code generations tools used to create the service stub for the new service.

If you have already created the service stub, please follow the instructions in the :doc:`developers guide <../developers>` to understand how to create the data path and the control path of your service.

Generating a service stub
-------------------------

``Pyang`` and ``swagger-codegen`` are used to generate a service stub using the YANG datamodel.
The former parses the YANG file and creates an intermediate JSON that is compliant with the `OpenAPI specifications <https://swagger.io/specification/>`_; the second starts from the latter file and creates a C++ code skeleton that actually implements the service. Among the services that are automatically provided, the C++ skeleton provides data validation and it handles the case in which a complex object is requested, such as the entire configuration of the service, which is returned by disaggregating the big request into a set of smaller requests for each _leaf_ object.
Hence, this leaves to the programmer only the responsibility to implement the interaction with _leaf_ objects.

Automatic installation
^^^^^^^^^^^^^^^^^^^^^^

The ``install-autocodegen-tools.sh`` script under this folder allows you to automatically install the new tools for the automatic code generation.

To run this script you first need to export the `POLYCUBE_HOME` variable and point it to the `polycube` main folder, as shown below:

::

  export POLYCUBE_HOME=<path-to-polycube-folder>

You can put this variable under ``~/.bashrc`` if you don't want to put it manually every time it is required.
Once completed you can run the script:

::

  cd devel
  ./install-autocodegen-tools.sh

The installation will take some time and at the end a successful message will be printed.
At this point you can directly generate the C++ service stub running the ``polycube-codegen`` tool.

::

  ~$ polycube-codegen
  You should specify the YANG file with the -i option
  polycube-codegen [-h] [-i input_yang] [-o output_folder]
  Polycube code generator that translates a YANG file into an polycube C++ service stub

  where:
    -h  show this help text
    -i  path to the input YANG file
    -o  path to the destination folder where the service stub will be placed


If you do not know what you are doing, it is strongly suggested to use the `polycube-codegen` tool to generate the service C++ stub.

Manual installation
^^^^^^^^^^^^^^^^^^^

Installing pyang
****************

Dependencies:

::

  sudo apt-get install python-minimal -y
  sudo apt-get install python-pip -y


Pyang:

::

  git clone -b polycube https://github.com/polycube-network/pyang-swagger.git
  cd pyang-swagger
  git checkout polycube
  sudo ./install.sh


Installing swagger-codegen
**************************

Dependencies:

::

  sudo apt-get install default-jre -y
  sudo apt-get install default-jdk -y
  sudo apt install maven -y

swagger-codegen:

::

  git clone -b polycube https://github.com/polycube-network/swagger-codegen.git
  cd swagger-codegen
  git checkout polycube
  mvn clean package # it takes some time

.. _codegen:

Generating the REST API from the YANG datamodel
***********************************************

This command generates a swagger description of the REST API.
Notice that this file could be used to generate client libraries for accessing the service API.

::

  pyang -f swagger <filename>.yang \
    -p <polycube/src/services/datamodel-common> \
    -o <output-swagger-file>.json


The -p parameter should point to the folder that contains the base model definition: `polycube/src/services/datamodel-common`.

Generating the C++ service stub from REST API

::

  cd swagger-codegen/modules/swagger-codegen-cli/target
  java -jar ./swagger-codegen-cli.jar generate -i <swagger-file>.json -l polycube -o .

Where `-o` indicates the destination folder of the generated files.

This command generates an empty service stub using the same structure as described in the service code structure section.
The developer only has to modify the `src/src/{object-name}.h`, `src/{object-name}.cpp` and `src/{service-name}_dp.h` files.

2b. Updating an existing service stub
*************************************

This step is required if the service skeleton already exists and the programmer has to update the YANG model with new features.
This procedure avoids to overwrite all the source code already generated, by adopting an incremental approach that minimizes the changes in the existing code to the developer.

Starting from the generated classes, the developer only has to modify the ``src/src/{object-name}.h``, ``src/{object-name}.cpp`` and `src/{service-name}_dp.h` files.

The ``src/{object-name}.cpp`` files contain the stub for the methods that the developer has to modify. Some of them have a basic implementation and the developer is suggested to improve it, others present only the stub. The developer has to implement constructor and destructor for each object. For each resources in the service the developer has to implement the `create`, `getEntry` and `removeEntry` static methods (in case of list also `get` and `remove` static methods are present), that respectively create the object inside  the parent object, retrieve the pointer for that object (or a list of pointer) and remove the object from the parent object (or a list of object). For example in order to add a `filteringdatabase` entry in a bridge service the developer has to implement the create static method in the class `Filteringdatabaseentry.cpp`.
Moreover in these classes the developer has to implement the getter and setter methods for each parameter contained in the object.
The methods `update` and `toJsonObject`, as mentioned previously, have a basic implementation.

In order to achieve the target, the developer is free to add fields and methods to these classes and also add other classes if necessary.

If the developer has to re-generate the service stub (because he modified something in datamodel), we suggest to avoid the re-generation of some files by properly modifying the `.swagger-codegen-ignore` file. All the files are considered in the `src/` folder (except for the files in `src/api/` folder). To avoid a file re-generation the developer has to write the file name in the `.swagger-codegen-ignore` file. If the file name is preceded by the `!` character or it is not present, the file will be overwritten. For default the `.swagger-codegen-ignore` avoids the re-generation of all the files that the developer has to modify (`src/src/{object-name}.h`, `src/{object-name}.cpp`  and `src/{service-name}_dp.h` files).

**Important**: do not remove the line ``.swagger-codegen-ignore`` present in the ``.swagger-codegen-ignore`` file because otherwise this file will be overwritten and this causes the loss of previous modifications to the file.




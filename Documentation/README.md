# Polycube Documentation

**The polycube documentation is available on [ReadTheDocs](https://polycube-network.readthedocs.io/en/latest/).**

The instructions below are intended only if you want to generate the documentation locally.

## Building the documentation locally

### Installing the dependencies

Our documentation uses `sphinx`.
In order to install the dependencies type:

```
cd Documentation
pip install -r requirements.txt
```

### Edit it

Documentation is created in the ReSTructured text format.
A nice document listing important commands is the [Restructured Text (reST) and Sphinx CheatSheet](https://thomas-cokelaer.info/tutorials/sphinx/rest_syntax.html).

Please note that you have mostly two ways to create a link to another location of the documentation:

- use an explicit reference to the target location (e.g., ```:ref:`sec-updating-linux-kernel` ```), which must declare a 'label' (e.g., ```.. _sec-updating-linux-kernel:``` followed by an empty line);

- use the ```:doc:`polycubectl CLI <polycubectl/polycubectl>` ``` primitive, which enables to refer to a specific file and allows to specify the text to be written in the link.

- use the ```:scm_web:``` macro, which avoids to declare the entire path of the linked document when this refers to the github repository (e.g., ``` :scm_web:`Nat <src/services/pcn-nat/src/Nat_dp.c>` ```)

### Build it

```
cd Documentation
make html
```

### Check spelling

```
cd Documentation
make spelling
```

The html documentation files will be in the `Documentation/_build` folder.

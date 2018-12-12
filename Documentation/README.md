# Polycube Documentation

*The polycube documentation is not designed to be read on github, so please see our website: TODO: put link here*

## Building the documentation

### Installing the dependencies

Our documentation uses `sphinx`.
In order to install the dependencies do

```
cd Documentation
pip install -r Documentation/requirements.txt
```

### Build it

```
cd Documentation
make html
```

### Check spelling

``
cd Documentation
make spelling
``

The html documentation files will be in the `Documentation/_build` folder.

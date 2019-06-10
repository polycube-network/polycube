# Iptables Service
`pcn-iptables` service is intended to emulate `iptables` using same semantic but different backend, based on `eBPF` programs and more efficients algorithms and runtime optimizations.

## Steps to INSTALL pcn-iptables

For `pcn-iptables` support you should enable `ENABLE_PCN_IPTABLES` flag in CMakeFile.
```
cd polycube
mkdir -p build
cd build
cmake .. -DENABLE_PCN_IPTABLES=ON
make -j`nproc` && sudo make install
```

## Steps to RUN pcn-iptables

### 1. Initialization, start `pcn-iptables` service

```
# Start polycubed, in other terminal (or background)
sudo polycubed &
# run pcn-iptables-init.
pcn-iptables-init
```

### 2. Use pcn-iptables, with same syntax of iptables
```
# E.g.
pcn-iptables -A INPUT -s 10.0.0.1 -j DROP # Append rule to INPUT chain
pcn-iptables -D INPUT -s 10.0.0.1 -j DROP # Delete rule from INPUT chain
pcn-iptables -I INPUT -s 10.0.0.2 -j DROP # Insert rule into INPUT chain

pcn-iptables -S # dump rules
pcn-iptables -L INPUT # dump rules for INPUT chain

pcn-iptables -P FORWARD DROP # set default policy for FORWARD chain
```

**NOTE**: do _not_ use use `sudo pcn-iptables ...`

### 3. Cleanup, stop `pcn-iptables` service

```
# run pcn-iptables-clean
pcn-iptables-clean

Validate it, if clean up successful:
# Execute pcn-iptables -S and you should receive "No cube found named pcn-iptables"
pcn-iptables -S
```


## pcn-iptables components

### iptables submodule

Under `src/components/iptables/iptables` a customized fork of iptables is included as submodule.
We customized this version of iptables in order not to inject iptables command into netfilter, but convert them, after a validation, into polycube syntax.

### scripts folder

Scripts are used as a glue logic to make pcn-iptables run. Main purpose is initialize, cleanup and run pcn-iptables, pass pcn-iptables parameters through iptables (in charge of converting them), then pass converted commands to pcn-iptables service.
Scripts are installed under `/usr/local/bin`.

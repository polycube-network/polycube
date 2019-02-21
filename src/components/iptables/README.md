# Iptables Service
`bpf-iptables` service is intended to emulate `iptables` using same semantic but different backend, based on `eBPF` programs and more efficients algorithms and runtime optimizations.

## Steps to INSTALL bpf-iptables

For `bpf-iptables` support you should enable `ENABLE_BPF_IPTABLES` flag in CMakeFile.
```
cd polycube/build/
cmake .. -DENABLE_BPF_IPTABLES=ON
make && make install
```

## Steps to RUN bpf-iptables

### 1. Initialization, start `bpf-iptables` service

```
# Start polycubed, in other terminal (or background)
sudo polycubed
# run bpf-iptables-init.
bpf-iptables-init
```

### 2. Use bpf-iptables, with same syntax of iptables
```
# E.g.
bpf-iptables -A INPUT -s 10.0.0.1 -j DROP # Append rule to INPUT chain
bpf-iptables -D INPUT -s 10.0.0.1 -j DROP # Delete rule from INPUT chain
bpf-iptables -I INPUT -s 10.0.0.2 -j DROP # Insert rule into INPUT chain

bpf-iptables -S # dump rules
bpf-iptables -L INPUT # dump rules for INPUT chain

bpf-iptables -P FORWARD DROP # set default policy for FORWARD chain
```

**NOTE**: do _not_ use use `sudo bpf-iptables ...`

### 3. Cleanup, stop `bpf-iptables` service

```
# run bpf-iptables-clean
bpf-iptables-clean
```


## bpf-iptables components

### iptables submodule

Under `src/components/iptables/iptables` a customized fork of iptables is included as submodule.
We customized this version of iptables in order not to inject iptables command into netfilter, but convert them, after a validation, into polycube syntax.

### scripts folder

Scripts are used as a glue logic to make bpf-iptables run. Main purpose is initialize, cleanup and run bpf-iptables, pass bpf-iptables parameters through iptables (in charge of converting them), then pass converted commands to bpf-iptables service.
Scripts are installed under `/usr/local/bin`.

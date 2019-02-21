bpf-iptables components
-----------------------

``bpf-iptables`` is composed by three main components:

1. ``bpf-iptables`` service (``src/services/bpf-iptables``): a Polycube service, a special one since performs some extra work, but basically expose its API and CLI, according to Polycube standard.

2. ``iptables*`` (``src/components/iptables/iptables``): a modified version of iptables, in charge of validate commands, translate them from iptables to polycube syntax, then forward them to bpf-iptables service instead of push them into the kernel via netfilter.

3. ``scripts`` (``src/components/iptables/scripts``): this is a folder containing some glue logic and scripts to initialize, cleanup and use ``bpf-iptables``. ``bpf-iptables`` itself is a script that forwards commands to ``iptables*`` (2), then forwards the translated command to ``bpf-iptables`` (1). Scripts are installed under ``/usr/local/bin``.

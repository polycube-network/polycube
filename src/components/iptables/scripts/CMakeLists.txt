set(bpf-iptables-files
  bpf-iptables
  bpf-iptables-init
  bpf-iptables-clean
  bpf-iptables-init-xdp
)

set(bpf-iptables-permissions
  OWNER_READ
  OWNER_WRITE
  OWNER_EXECUTE
  GROUP_READ
  GROUP_EXECUTE
  WORLD_READ
  WORLD_EXECUTE
)

install(FILES ${bpf-iptables-files}
        PERMISSIONS ${bpf-iptables-permissions}
        DESTINATION /usr/local/bin/)

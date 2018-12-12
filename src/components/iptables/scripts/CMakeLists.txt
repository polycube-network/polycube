set(pcn-iptables-files
  pcn-iptables
  pcn-iptables-init
  pcn-iptables-clean
  pcn-iptables-init-xdp
)

set(pcn-iptables-permissions
  OWNER_READ
  OWNER_WRITE
  OWNER_EXECUTE
  GROUP_READ
  GROUP_EXECUTE
  WORLD_READ
  WORLD_EXECUTE
)

install(FILES ${pcn-iptables-files}
        PERMISSIONS ${pcn-iptables-permissions}
        DESTINATION /usr/local/bin/)

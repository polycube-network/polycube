#define NATTYPE NATTYPE_EGRESS
// defined in ingress program
BPF_TABLE("extern", struct st_k, struct st_v, egress_session_table,
          NAT_MAP_DIM);
BPF_TABLE("extern", struct st_k, struct st_v, ingress_session_table,
          NAT_MAP_DIM);
// only needed in egress
// SNAT + MASQUERADE rules
struct sm_k {
  u32 internal_netmask_len;
  __be32 internal_ip;
};
struct sm_v {
  __be32 external_ip;
  uint8_t entry_type;
};
BPF_F_TABLE("lpm_trie", struct sm_k, struct sm_v, sm_rules, 1024,
            BPF_F_NO_PREALLOC);
// Port numbers
BPF_TABLE("array", u32, u16, first_free_port, 1);
static inline __be16 get_free_port() {
  u32 i = 0;
  u16 *new_port_p = first_free_port.lookup(&i);
  if (!new_port_p)
    return 0;
  rcu_read_lock();
  if (*new_port_p < 1024 || *new_port_p == 65535)
    *new_port_p = 1024;
  *new_port_p = *new_port_p + 1;
  rcu_read_unlock();
  return bpf_htons(*new_port_p);
}
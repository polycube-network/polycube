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
struct free_port_entry {
  u16 first_free_port;
  struct bpf_spin_lock lock;
};
BPF_TABLE("array", u32, struct free_port_entry, first_free_port, 1);
static inline __be16 get_free_port() {
  u32 i = 0;
  u16 port = 0;
  struct free_port_entry *entry = first_free_port.lookup(&i);
  if (!entry)
    return 0;
  bpf_spin_lock(&entry->lock);
  if (entry->first_free_port < 1024 || entry->first_free_port == 65535)
    entry->first_free_port = 1024;
  port = entry->first_free_port;
  entry->first_free_port++;
  bpf_spin_unlock(&entry->lock);
  return bpf_htons(port);
}
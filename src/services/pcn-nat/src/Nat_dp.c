static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
  // NAT processing happens in 4 steps:
  // 1) packet parsing
  // 2) session table lookup
  // 3) rule lookup
  // 4) packet modification

  // Parse packet
  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;

  struct eth_hdr *eth = data;
  if ( (void *)eth + sizeof(*eth) > data_end )
    goto DROP;

  pcn_log(
      ctx, LOG_TRACE,
      "Received new packet: in_port: %d eth_type: 0x%x mac_src: %M mac_dst: %M",
      md->in_port, bpf_ntohs(eth->proto), eth->src, eth->dst);

  switch (eth->proto) {
  case htons(ETH_P_IP):
    // Packet is IP
    pcn_log(ctx, LOG_DEBUG, "Received IP Packet");
    break;
  case htons(ETH_P_ARP):
    // Packet is ARP: let is pass
    pcn_log(ctx, LOG_TRACE, "Received ARP packet. Letting it go through");
    return RX_OK;
  default:
    pcn_log(ctx, LOG_TRACE, "Unknown eth proto: %d, dropping",
            bpf_ntohs(eth->proto));
    goto DROP;
  }

  // Packet data
  uint32_t srcIp = 0;
  uint32_t dstIp = 0;
  uint16_t srcPort = 0;
  uint16_t dstPort = 0;
  uint8_t proto = 0;

  // Nat data
  uint32_t newIp = 0;
  uint16_t newPort = 0;
  uint8_t rule_type = 0;

  // Status data
  uint8_t update_session_table = 1;

  struct iphdr *ip = data + sizeof(*eth);
  if ( (void *)ip + sizeof(*ip) > data_end )
    goto DROP;

  pcn_log(ctx, LOG_TRACE, "Processing IP packet: src %I, dst: %I", ip->saddr,
          ip->daddr);

  srcIp = ip->saddr;
  dstIp = ip->daddr;
  proto = ip->protocol;

  switch (ip->protocol) {
  case IPPROTO_TCP: {
    uint8_t header_len = 4 * ip->ihl;
    struct tcphdr *tcp = data + sizeof(*eth) + header_len;
    if ( (void *)tcp + sizeof(*tcp) > data_end )
      goto DROP;

    pcn_log(ctx, LOG_TRACE, "Packet is TCP: src_port %P, dst_port %P",
            tcp->source, tcp->dest);
    srcPort = tcp->source;
    dstPort = tcp->dest;
    break;
  }
  case IPPROTO_UDP: {
    uint8_t header_len = 4 * ip->ihl;
    struct udphdr *udp = data + sizeof(*eth) + header_len;
    if ( (void *)udp + sizeof(*udp) > data_end )
      goto DROP;
    pcn_log(ctx, LOG_TRACE, "Packet is UDP: src_port %P, dst_port %P",
            udp->source, udp->dest);
    srcPort = udp->source;
    dstPort = udp->dest;
    break;
  }
  case IPPROTO_ICMP: {
    uint8_t header_len = 4 * ip->ihl;
    struct icmphdr *icmp = data + sizeof(*eth) + header_len;
    if ( (void *)icmp + sizeof(*icmp) > data_end )
      goto DROP;
    pcn_log(ctx, LOG_TRACE, "Packet is ICMP: type %d, id %d", icmp->type,
            icmp->un.echo.id);

    // Consider the ICMP ID as a "port" number for easier handling
    srcPort = icmp->un.echo.id;
    dstPort = icmp->un.echo.id;
    break;
  }
  default:
    pcn_log(ctx, LOG_TRACE, "Unknown L4 proto %d, dropping", ip->protocol);
    goto DROP;
  }

  // Packet parsed, start session table lookup
  struct st_k key = {0, 0, 0, 0, 0};
  key.src_ip = srcIp;
  key.dst_ip = dstIp;
  key.src_port = srcPort;
  key.dst_port = dstPort;
  key.proto = proto;
  struct st_v *value;

#if NATTYPE == NATTYPE_EGRESS
  // Packet is inside -> outside, check egress session table
  value = egress_session_table.lookup(&key);
  if (value != NULL) {
    // Session table hit
    pcn_log(ctx, LOG_TRACE, "Egress session table: hit");

    newIp = value->new_ip;
    newPort = value->new_port;
    rule_type = NAT_SRC;

    update_session_table = 0;

    goto apply_nat;
  }
  pcn_log(ctx, LOG_TRACE, "Egress session table: miss");
//  } else {
#elif NATTYPE == NATTYPE_INGRESS
  // Packet is outside -> inside, check ingress session table
  value = ingress_session_table.lookup(&key);
  if (value != NULL) {
    // Session table hit
    pcn_log(ctx, LOG_TRACE, "Ingress session table: hit");

    newIp = value->new_ip;
    newPort = value->new_port;
    rule_type = NAT_DST;

    update_session_table = 0;

    goto apply_nat;
  }
  pcn_log(ctx, LOG_TRACE, "Ingress session table: miss");
//  }
#else
#error "Invalid NATTYPE"
#endif
// Session table miss, start rule lookup

#if NATTYPE == NATTYPE_EGRESS
  {
    // Packet is inside -> outside, check SNAT/MASQUERADE rule table
    struct sm_k key = {0, 0};
    key.internal_netmask_len = 32;
    key.internal_ip = srcIp;
    struct sm_v *value = sm_rules.lookup(&key);
    if (value != NULL) {
      pcn_log(ctx, LOG_TRACE, "Egress rule table: hit");

      newIp = value->external_ip;
      newPort = get_free_port();
      rule_type = value->entry_type;

      goto apply_nat;
    }
    pcn_log(ctx, LOG_TRACE, "Egress rule table: miss");
  }
//  } else {
#elif NATTYPE == NATTYPE_INGRESS
  // Packet is outside -> inside, check DNAT/PORTFORWARDING rule table
  {
    struct dp_k key = {0, 0, 0};
    key.mask = 56;  // 32 (IP) + 16 (Port) + 8 (Proto)
    key.external_ip = dstIp;
    key.external_port = dstPort;
    key.proto = proto;
    struct dp_v *value = dp_rules.lookup(&key);
    if (value != NULL) {
      pcn_log(ctx, LOG_TRACE, "Ingress rule table: hit");

      newIp = value->internal_ip;
      newPort = value->internal_port;
      rule_type = value->entry_type;
      if (newPort == 0) {
        // Matching rule is DNAT, keep the same port number
        newPort = dstPort;
      }

      goto apply_nat;
    }
    pcn_log(ctx, LOG_TRACE, "Ingress rule table: miss");
  }
//  }
#else
#error "Invalid NATTYPE"
#endif
  // No matching entry was found in the session tables
  // No matching rule was found in the rule tables
  // -> Forward packet as it is
  goto proceed;

apply_nat:;
  if (update_session_table == 1) {
    // No session table exist for the packet, but a rule matched
    // -> Update the session tables in both directions

    struct st_k forward_key = {0, 0, 0, 0, 0};
    struct st_v forward_value = {0, 0};

    struct st_k reverse_key = {0, 0, 0, 0, 0};
    struct st_v reverse_value = {0, 0};

    if (rule_type == NAT_SRC || rule_type == NAT_MSQ) {
      // A rule matched in the inside -> outside direction

      // Session table entry for the outgoing packets
      forward_key.src_ip = srcIp;
      forward_key.dst_ip = dstIp;
      forward_key.src_port = srcPort;
      forward_key.dst_port = dstPort;
      forward_key.proto = proto;

      forward_value.new_ip = newIp;
      forward_value.new_port = newPort;
      forward_value.originating_rule_type = rule_type;

      // Session table entry for the incoming packets
      reverse_key.src_ip = dstIp;
      reverse_key.dst_ip = newIp;
      if (proto == IPPROTO_ICMP) {
        // For ICMP session table entries "source port" and "destination port"
        // must be the same, equal to the ICMP ID
        reverse_key.src_port = newPort;
      } else {
        reverse_key.src_port = dstPort;
      }
      reverse_key.dst_port = newPort;
      reverse_key.proto = proto;

      reverse_value.new_ip = srcIp;
      reverse_value.new_port = srcPort;
      reverse_value.originating_rule_type = rule_type;

      pcn_log(ctx, LOG_TRACE, "Updating session tables after SNAT");
      pcn_log(ctx, LOG_DEBUG, "New outgoing connection: %I:%P -> %I:%P", srcIp,
              srcPort, dstIp, dstPort);
    } else {
      // A rule matched in the outside -> inside direction

      // Session table entry for the outgoing packets
      forward_key.src_ip = newIp;
      forward_key.dst_ip = srcIp;
      forward_key.src_port = newPort;
      if (proto == IPPROTO_ICMP) {
        // For ICMP session table entries "source port" and "destination port"
        // must be the same, equal to the ICMP ID
        forward_key.dst_port = newPort;
      } else {
        forward_key.dst_port = srcPort;
      }
      forward_key.proto = proto;

      forward_value.new_ip = dstIp;
      forward_value.new_port = dstPort;
      forward_value.originating_rule_type = rule_type;

      // Session table entry for the incoming packets
      reverse_key.src_ip = srcIp;
      reverse_key.dst_ip = dstIp;
      reverse_key.src_port = srcPort;
      reverse_key.dst_port = dstPort;
      reverse_key.proto = proto;

      reverse_value.new_ip = newIp;
      reverse_value.new_port = newPort;
      reverse_value.originating_rule_type = rule_type;

      pcn_log(ctx, LOG_TRACE, "Updating session tables after DNAT");
      pcn_log(ctx, LOG_DEBUG, "New incoming connection: %I:%P -> %I:%P", srcIp,
              srcPort, dstIp, dstPort);
    }
    egress_session_table.update(&forward_key, &forward_value);
    ingress_session_table.update(&reverse_key, &reverse_value);

    pcn_log(ctx, LOG_TRACE,
            "Using ingress key: srcIp %I, dstIp %I, srcPort %P, dstPort %P",
            reverse_key.src_ip, reverse_key.dst_ip, reverse_key.src_port,
            reverse_key.dst_port);
    pcn_log(ctx, LOG_TRACE,
            "Using egress key: srcIp %I, dstIp %I, srcPort %P, dstPort %P",
            forward_key.src_ip, forward_key.dst_ip, forward_key.src_port,
            forward_key.dst_port);
  }

  // Modify packet
  uint32_t old_ip =
      (rule_type == NAT_SRC || rule_type == NAT_MSQ) ? srcIp : dstIp;
  uint32_t old_port =
      (rule_type == NAT_SRC || rule_type == NAT_MSQ) ? srcPort : dstPort;
  uint32_t new_ip = newIp;
  uint32_t new_port = newPort;
  uint32_t l3sum = pcn_csum_diff(&old_ip, 4, &new_ip, 4, 0);
  uint32_t l4sum = pcn_csum_diff(&old_port, 4, &new_port, 4, 0);
  switch (proto) {
  case IPPROTO_TCP: {
    uint8_t header_len = 4 * ip->ihl;
    struct tcphdr *tcp = data + sizeof(*eth) + header_len;
    if ( (void *)tcp + sizeof(*tcp) > data_end )
      goto DROP;

    if (rule_type == NAT_SRC || rule_type == NAT_MSQ) {
      ip->saddr = new_ip;
      tcp->source = (__be16)new_port;
      pcn_log(ctx, LOG_TRACE, "Natted TCP packet: source, %I:%P -> %I:%P",
              old_ip, old_port, new_ip, new_port);
    } else {
      ip->daddr = new_ip;
      tcp->dest = (__be16)new_port;
      pcn_log(ctx, LOG_TRACE, "Natted TCP packet: destination, %I:%P -> %I:%P",
              old_ip, old_port, new_ip, new_port);
    }

    // Update checksums
    pcn_l4_csum_replace(ctx, TCP_CSUM_OFFSET, 0, l3sum, IS_PSEUDO | 0);
    pcn_l4_csum_replace(ctx, TCP_CSUM_OFFSET, 0, l4sum, 0);
    pcn_l3_csum_replace(ctx, IP_CSUM_OFFSET, 0, l3sum, 0);

    goto proceed;
  }
  case IPPROTO_UDP: {
    uint8_t header_len = 4 * ip->ihl;
    struct udphdr *udp = data + sizeof(*eth) + header_len;
    if ( (void *)udp + sizeof(*udp) > data_end )
      goto DROP;
    if (rule_type == NAT_SRC || rule_type == NAT_MSQ) {
      ip->saddr = new_ip;
      udp->source = (__be16)new_port;
      pcn_log(ctx, LOG_TRACE, "Natted UDP packet: source, %I:%P -> %I:%P",
              old_ip, old_port, new_ip, new_port);
    } else {
      ip->daddr = new_ip;
      udp->dest = (__be16)new_port;
      pcn_log(ctx, LOG_TRACE, "Natted UDP packet: destination, %I:%P -> %I:%P",
              old_ip, old_port, new_ip, new_port);
    }

    // Update checksums
    pcn_l4_csum_replace(ctx, UDP_CSUM_OFFSET, 0, l3sum, IS_PSEUDO | 0);
    pcn_l4_csum_replace(ctx, UDP_CSUM_OFFSET, 0, l4sum, 0);
    pcn_l3_csum_replace(ctx, IP_CSUM_OFFSET, 0, l3sum, 0);

    goto proceed;
  }
  case IPPROTO_ICMP: {
    uint8_t header_len = 4 * ip->ihl;
    struct icmphdr *icmp = data + sizeof(*eth) + header_len;
    if ( (void *)icmp + sizeof(*icmp) > data_end )
      goto DROP;
    if (rule_type == NAT_SRC || rule_type == NAT_MSQ) {
      ip->saddr = new_ip;
      icmp->un.echo.id = (__be16)new_port;
      pcn_log(ctx, LOG_TRACE, "Natted ICMP packet: source, %I:%P -> %I:%P",
              old_ip, old_port, new_ip, new_port);
    } else {
      ip->daddr = new_ip;
      icmp->un.echo.id = (__be16)new_port;
      pcn_log(ctx, LOG_TRACE, "Natted ICMP packet: destination, %I:%P -> %I:%P",
              old_ip, old_port, new_ip, new_port);
    }

    // Update checksums
    pcn_l4_csum_replace(ctx, ICMP_CSUM_OFFSET, 0, l4sum, 0);
    pcn_l3_csum_replace(ctx, IP_CSUM_OFFSET, 0, l3sum, 0);

    goto proceed;
  }
  }
proceed:;
  return RX_OK;
DROP:;
  pcn_log(ctx, LOG_INFO, "Dropping packet");
  return RX_DROP;
}
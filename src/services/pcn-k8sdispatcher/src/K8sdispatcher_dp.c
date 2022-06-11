/*
 * This function is called each time a packet arrives to the cube.
 * ctx contains the packet and md some additional metadata for the packet.
 * If the service is of type XDP_SKB/DRV CTX TYPE is equivalent to the struct
 * xdp_md otherwise, if the service is of type TC, CTXTYPE is equivalent to
 * the __sk_buff struct
 * Please look at the libpolycube documentation for more details.
 */
#include <linux/jhash.h>
#include <uapi/linux/icmp.h>
#include <uapi/linux/if_arp.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/tcp.h>
#include <uapi/linux/udp.h>
#include <uapi/linux/bpf.h>
#include <uapi/linux/filter.h>
#include <uapi/linux/if_ether.h>
#include <uapi/linux/if_packet.h>
#include <uapi/linux/in.h>
#include <uapi/linux/pkt_cls.h>

#ifndef NAT_SRC
#define NAT_SRC 0
#endif

#ifndef NAT_DST
#define NAT_DST 1
#endif

#ifndef NODEPORT_LOCAL
#define NODEPORT_LOCAL 3
#endif

#ifndef NODEPORT_CLUSTER
#define NODEPORT_CLUSTER 4
#endif

#ifndef FRONTEND_PORT
#define FRONTEND_PORT 0
#endif

#ifndef BACKEND_PORT
#define BACKEND_PORT 1
#endif

#ifndef NODEPORT_RANGE_LOW
#define NODEPORT_RANGE_LOW (30000)
#endif

#ifndef NODEPORT_RANGE_HIGH
#define NODEPORT_RANGE_HIGH (32767)
#endif

#ifndef EXTERNAL_IP
#define EXTERNAL_IP 0
#endif

#ifndef NATTED_IP
#define NATTED_IP 0
#endif

#define NAT_MAP_DIM 32768

#define REASON_ARP_REPLY 0x0

#define IP_CSUM_OFFSET (sizeof(struct eth_hdr) + offsetof(struct iphdr, check))
#define UDP_CSUM_OFFSET                            \
  (sizeof(struct eth_hdr) + sizeof(struct iphdr) + \
   offsetof(struct udphdr, check))
#define TCP_CSUM_OFFSET                            \
  (sizeof(struct eth_hdr) + sizeof(struct iphdr) + \
   offsetof(struct tcphdr, check))
#define ICMP_CSUM_OFFSET                           \
  (sizeof(struct eth_hdr) + sizeof(struct iphdr) + \
   offsetof(struct icmphdr, checksum))
#define IS_PSEUDO 0x10

struct eth_hdr {
    __be64 dst: 48;
    __be64 src: 48;
    __be16 proto;
} __attribute__((packed));

struct arp_hdr {
    __be16 ar_hrd;        /* format of hardware address	*/
    __be16 ar_pro;        /* format of protocol address	*/
    unsigned char ar_hln; /* length of hardware address	*/
    unsigned char ar_pln; /* length of protocol address	*/
    __be16 ar_op;         /* ARP opcode (command)		*/
    __be64 ar_sha: 48;   /* sender hardware address	*/
    __be32 ar_sip;        /* sender IP address		*/
    __be64 ar_tha: 48;   /* target hardware address	*/
    __be32 ar_tip;        /* target IP address		*/
} __attribute__((packed));

// Session table
struct st_k {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t proto;
};
struct st_v {
    uint32_t new_ip;
    uint16_t new_port;
    uint8_t originating_rule_type;
};
BPF_TABLE("lru_hash", struct st_k, struct st_v, egress_session_table,
NAT_MAP_DIM);
BPF_TABLE("lru_hash", struct st_k, struct st_v, ingress_session_table,
NAT_MAP_DIM);

// DNAT + PORTFORWARDING rules
struct dp_k {
    __be64 dummy;
    __be16 external_port;
    __be16 proto;
};
struct dp_v {
    __be16 internal_port;
    __be16 entry_type;
};

BPF_F_TABLE("lpm_trie", struct dp_k, struct dp_v, dp_rules, 1024,
BPF_F_NO_PREALLOC);

// Port numbers
BPF_TABLE("array", u32, u16, first_free_port, 1);

static inline __be16

get_free_port() {
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

static int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
    // NAT processing happens in 4 steps:
    // 1) packet parsing
    // 2) session table lookup
    // 3) rule lookup
    // 4) packet modification

    // Parse packet
    void *data = (void *) (long) ctx->data;
    void *data_end = (void *) (long) ctx->data_end;

    struct eth_hdr *eth = data;
    if (data + sizeof(*eth) > data_end) {
        pcn_log(ctx, LOG_TRACE, "dropped Ethernet pkt - (in_port: %d) (reason: inconsistent_size)", md->in_port);
        return RX_DROP;
    }

    pcn_log(
            ctx, LOG_TRACE,
            "received new pkt: in_port: %d eth_type: 0x%x mac_src: %M mac_dst: %M",
            md->in_port, bpf_ntohs(eth->proto), eth->src, eth->dst);


    // Handling ARP packet
    switch (eth->proto) {
        case htons(ETH_P_IP):
            goto IP;  // ipv4 packet
        case htons(ETH_P_ARP):
            goto ARP;  // arp packet
        default: {
            pcn_log(ctx, LOG_TRACE, "pkt is not an IPv4/ARP pkt: sent to stack - (eth_proto 0x%x)",
                    bpf_ntohs(eth->proto));
            return RX_OK;
        }
    }

    IP:;
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
    if (data + sizeof(*eth) + sizeof(*ip) > data_end) {
        pcn_log(ctx, LOG_TRACE, "dropped IP pkt - (in_port: %d) (reason: inconsistent_size)", md->in_port);
        return RX_DROP;
    }

    // check if ip dest is for this host
    if (md->in_port == FRONTEND_PORT && ip->daddr != EXTERNAL_IP) {
        pcn_log(
                ctx, LOG_TRACE,
                "pkt coming from FRONTEND port is not for host: sent to stack - (in_port: %d) (dst_ip: %I)",
                md->in_port, ip->daddr
        );
        return RX_OK;
    }

    pcn_log(ctx, LOG_TRACE, "pkt is a valid IP pkt: processing... - (in_port: %d)", md->in_port);

    srcIp = ip->saddr;
    dstIp = ip->daddr;
    proto = ip->protocol;

    switch (ip->protocol) {
        case IPPROTO_TCP: {
            struct tcphdr *tcp = data + sizeof(*eth) + sizeof(*ip);
            if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*tcp) > data_end) {
                pcn_log(ctx, LOG_TRACE, "dropped TCP pkt - (in_port: %d) (reason: inconsistent_size)", md->in_port);
                return RX_DROP;
            }
            pcn_log(ctx, LOG_TRACE, "pkt is a valid TCP pkt: processing... - (in_port: %d)", md->in_port);

            srcPort = tcp->source;
            dstPort = tcp->dest;
            break;
        }
        case IPPROTO_UDP: {
            struct udphdr *udp = data + sizeof(*eth) + sizeof(*ip);
            if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*udp) > data_end) {
                pcn_log(ctx, LOG_TRACE, "dropped UDP pkt - (in_port: %d) (reason: inconsistent_size)", md->in_port);
                return RX_DROP;
            }
            pcn_log(ctx, LOG_TRACE, "pkt is a valid UDP pkt: processing... - (in_port: %d)", md->in_port);

            srcPort = udp->source;
            dstPort = udp->dest;
            break;
        }
        case IPPROTO_ICMP: {
            struct icmphdr *icmp = data + sizeof(*eth) + sizeof(*ip);
            if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*icmp) > data_end) {
                pcn_log(ctx, LOG_TRACE, "dropped ICMP pkt - (in_port: %d) (reason: inconsistent_size)", md->in_port);
                return RX_DROP;
            }
            pcn_log(ctx, LOG_TRACE, "pkt is a valid ICMP pkt: processing... - (in_port: %d)", md->in_port);

            // Consider the ICMP ID as a "port" number for easier handling
            srcPort = icmp->un.echo.id;
            dstPort = icmp->un.echo.id;
            break;
        }
        default:
            pcn_log(
                    ctx, LOG_TRACE,
                    "dropped IP pkt - (in_port: %d) (reason: unsupported_type) (ip_proto: %d)",
                    md->in_port, ip->protocol
            );
            return RX_DROP;
    }

    // Packet parsed, start session table lookup
    struct st_k key = {0, 0, 0, 0, 0};
    key.src_ip = srcIp;
    key.dst_ip = dstIp;
    key.src_port = srcPort;
    key.dst_port = dstPort;
    key.proto = proto;
    struct st_v *value;

    if (md->in_port == BACKEND_PORT) {
        // Packet is inside -> outside, check egress session table
        pcn_log(
                ctx, LOG_TRACE, "checking egress session table... - (src: %I:%P, dst: %I:%P)",
                srcIp, srcPort, dstIp, dstPort
        );
        value = egress_session_table.lookup(&key);
        if (value != NULL) {
            // Session table hit
            newIp = value->new_ip;
            newPort = value->new_port;

            if (srcIp == EXTERNAL_IP && ntohs(srcPort) >= NODEPORT_RANGE_LOW && ntohs(srcPort) <= NODEPORT_RANGE_HIGH) {
                rule_type = NAT_DST;
                pcn_log(ctx, LOG_TRACE,
                        "egress session table hit for a NodePort pkt: chosen NAT_DST - (src: %I:%P, dst: %I:%P)",
                        srcIp, srcPort, dstIp, dstPort
                );
                pcn_log(ctx, LOG_TRACE, "NAT_DST - (dst: %I:%P) (new_dst: %I:%P)", dstIp, dstPort, newIp, newPort);
            } else {
                rule_type = NAT_SRC;
                pcn_log(ctx, LOG_TRACE,
                        "egress session table hit for a non NodePort pkt: chosen NAT_SRC - (src: %I:%P, dst: %I:%P)",
                        srcIp, srcPort, dstIp, dstPort
                );
                pcn_log(ctx, LOG_TRACE, "NAT_SRC - (src: %I:%P) (new_src: %I:%P)", srcIp, srcPort, newIp, newPort);
            }

            update_session_table = 0;

            goto apply_nat;
        }
        pcn_log(
                ctx, LOG_TRACE, "egress session table miss - (src: %I:%P, dst: %I:%P)",
                srcIp, srcPort, dstIp, dstPort
        );
    } else {
        // Packet is outside -> inside, check ingress session table
        pcn_log(
                ctx, LOG_TRACE, "checking ingress session table... - (src: %I:%P, dst: %I:%P)",
                srcIp, srcPort, dstIp, dstPort
        );
        value = ingress_session_table.lookup(&key);
        if (value != NULL) {
            // Session table hit
            newIp = value->new_ip;
            newPort = value->new_port;

            if (value->originating_rule_type == NODEPORT_CLUSTER) {
                rule_type = NAT_SRC;
                pcn_log(ctx, LOG_TRACE,
                        "ingress session table hit for a NodePort (CLUSTER) service: chosen NAT_SRC - (src: %I:%P, dst: %I:%P)",
                        srcIp, srcPort, dstIp, dstPort
                );
                pcn_log(ctx, LOG_TRACE, "NAT_SRC - (src: %I:%P) (new_src: %I:%P)", srcIp, srcPort, newIp, newPort);
            } else {
                rule_type = NAT_DST;
                pcn_log(
                        ctx, LOG_TRACE,
                        "ingress session table hit for a non NodePort (CLUSTER) service: chosen NAT_DST - (src: %I:%P, dst: %I:%P)",
                        srcIp, srcPort, dstIp, dstPort
                );
                pcn_log(ctx, LOG_TRACE, "NAT_DST - (dst: %I:%P) (new_dst: %I:%P)", dstIp, dstPort, newIp, newPort);
            }

            update_session_table = 0;

            goto apply_nat;
        }
        pcn_log(
                ctx, LOG_TRACE, "ingress session table miss - (src: %I:%P, dst: %I:%P)",
                srcIp, srcPort, dstIp, dstPort
        );
    }

    // Session table miss, start rule lookup
    if (md->in_port == BACKEND_PORT) {
        // Packet is inside -> outside, check SNAT/MASQUERADE rule table
        pcn_log(
                ctx, LOG_TRACE, "checking egress rules... - (src: %I:%P, dst: %I:%P)",
                srcIp, srcPort, dstIp, dstPort
        );
        if (srcIp == EXTERNAL_IP) {
            pcn_log(ctx, LOG_TRACE, "no egress rule match - (src: %I:%P, dst: %I:%P)", srcIp, srcPort, dstIp, dstPort);
            pcn_log(
                    ctx, LOG_TRACE,
                    "redirected pkt as is to FRONTEND port - (dst: %I:%P) (out_port: %d)",
                    dstIp, dstPort, FRONTEND_PORT
            );
            return pcn_pkt_redirect(ctx, md, FRONTEND_PORT);
        }

        newIp = EXTERNAL_IP;
        newPort = get_free_port();
        pcn_log(
                ctx, LOG_TRACE, "matched egress rule: chosen NAT_SRC - (src: %I:%P, dst: %I:%P)",
                srcIp, srcPort, dstIp, dstPort
        );
        pcn_log(ctx, LOG_TRACE, "NAT_SRC - (src: %I:%P) (new_src: %I:%P)", srcIp, srcPort, newIp, newPort);

    } else {
        // Packet is outside -> inside, check DNAT/PORTFORWARDING rule table
        struct dp_k key = {56, 0, 0};
        pcn_log(
                ctx, LOG_TRACE, "checking ingress rules... - (src: %I:%P, dst: %I:%P)",
                srcIp, srcPort, dstIp, dstPort
        );
        key.external_port = dstPort;
        key.proto = proto;
        struct dp_v *value = dp_rules.lookup(&key);
        if (value == NULL) {
            pcn_log(
                    ctx, LOG_TRACE, "no ingress rule match: redirected as is to stack - (src: %I:%P, dst: %I:%P)",
                    srcIp, srcPort, dstIp, dstPort
            );
            return RX_OK;
        }

        rule_type = value->entry_type;
        if (rule_type == NODEPORT_LOCAL) {
            pcn_log(
                    ctx, LOG_TRACE,
                    "matched ingress rule: NodePort (LOCAL) service pkt forwarded as is to BACKEND port - (src: %I:%P, dst: %I:%P)",
                    srcIp, srcPort, dstIp, dstPort
            );
            return pcn_pkt_redirect(ctx, md, BACKEND_PORT);
        }

        newIp = NATTED_IP;
        newPort = get_free_port();
        pcn_log(
                ctx, LOG_TRACE, "matched ingress rule: chosen NAT_SRC - (src: %I:%P, dst: %I:%P)",
                srcIp, srcPort, dstIp, dstPort
        );
        pcn_log(ctx, LOG_TRACE, "NAT_SRC - (src: %I:%P) (new_src: %I:%P)", srcIp, srcPort, newIp, newPort);

    }

    apply_nat:;
    if (update_session_table == 1) {
        // No session table exist for the packet, but a rule matched
        // -> Update the session tables in both directions

        struct st_k forward_key = {0, 0, 0, 0, 0};
        struct st_v forward_value = {0, 0};

        struct st_k reverse_key = {0, 0, 0, 0, 0};
        struct st_v reverse_value = {0, 0};

        if (rule_type == NODEPORT_CLUSTER) {
            // Session table entry for the incoming packets (ingress table)
            reverse_key.src_ip = srcIp;
            reverse_key.dst_ip = dstIp;
            reverse_key.src_port = srcPort;
            reverse_key.dst_port = dstPort;
            reverse_key.proto = proto;

            //nat_ip porta 1025 nodeport_cluster
            reverse_value.new_ip = newIp;
            reverse_value.new_port = newPort;
            reverse_value.originating_rule_type = rule_type;

            // 1 google -> nodeport   2 nodeport -> nat
            //sorgente -> nat
            // Session table entry for the outcoming packets (egress)
            //
            forward_key.src_ip = dstIp;
            forward_key.dst_ip = newIp;
            forward_key.src_port = dstPort;
            forward_key.dst_port = newPort;
            forward_key.proto = proto;

            //
            forward_value.new_ip = srcIp;
            forward_value.new_port = srcPort;
            forward_value.originating_rule_type = rule_type;

            pcn_log(ctx, LOG_INFO, "updating session tables for NodePort");
            pcn_log(ctx, LOG_INFO, "new outgoing connection: %I:%P -> %I:%P", srcIp,
                    srcPort, dstIp, dstPort);

            pcn_log(ctx, LOG_INFO,
                    "using ingress value: newIp %I, newPort %P", reverse_value.new_ip, reverse_value.new_port);
            pcn_log(ctx, LOG_INFO,
                    "using egress value: newIp %I, newPort %P", forward_value.new_ip, forward_value.new_port);

            rule_type = NAT_SRC;
        } else if (rule_type == NAT_SRC) {
            // A rule matched in the inside -> outside direction

            // Session table entry for the outgoing packets (egress table)
            forward_key.src_ip = srcIp;
            forward_key.dst_ip = dstIp;
            forward_key.src_port = srcPort;
            forward_key.dst_port = dstPort;
            forward_key.proto = proto;

            //nat_ip porta 1025 nodeport_cluster
            forward_value.new_ip = newIp;
            forward_value.new_port = newPort;
            forward_value.originating_rule_type = rule_type;

            //sorgente -> nat
            // Session table entry for the incoming packets (ingress)
            //
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

            //
            reverse_value.new_ip = srcIp;
            reverse_value.new_port = srcPort;
            reverse_value.originating_rule_type = rule_type;

            pcn_log(ctx, LOG_INFO, "updating session tables after SNAT");
            pcn_log(ctx, LOG_INFO, "new outgoing connection: %I:%P -> %I:%P", srcIp,
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

            pcn_log(ctx, LOG_INFO, "updating session tables after DNAT");
            pcn_log(ctx, LOG_INFO, "new incoming connection: %I:%P -> %I:%P", srcIp,
                    srcPort, dstIp, dstPort);
        }

        egress_session_table.update(&forward_key, &forward_value);
        ingress_session_table.update(&reverse_key, &reverse_value);

        pcn_log(ctx, LOG_INFO,
                "using ingress key: srcIp %I, dstIp %I, srcPort %P, dstPort %P",
                reverse_key.src_ip, reverse_key.dst_ip, reverse_key.src_port,
                reverse_key.dst_port);
        pcn_log(ctx, LOG_INFO,
                "using egress key: srcIp %I, dstIp %I, srcPort %P, dstPort %P",
                forward_key.src_ip, forward_key.dst_ip, forward_key.src_port,
                forward_key.dst_port);
    }

    // Modify packet
    uint32_t old_ip, old_port;
    if (rule_type == NAT_SRC) {
        old_ip = srcIp;
        old_port = srcPort;
    } else {
        old_ip = dstIp;
        old_port = dstPort;
    }
    uint32_t new_ip = newIp;
    uint32_t new_port = newPort;

    uint32_t l3sum = pcn_csum_diff(&old_ip, 4, &new_ip, 4, 0);
    uint32_t l4sum = pcn_csum_diff(&old_port, 4, &new_port, 4, 0);

    switch (proto) {
        case IPPROTO_TCP: {
            struct tcphdr *tcp = data + sizeof(*eth) + sizeof(*ip);
            if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*tcp) > data_end) {
                pcn_log(ctx, LOG_TRACE,
                        "dropped TCP pkt after new checksum evaluation - (in_port: %d) (reason: inconsistent_size)",
                        md->in_port);
                return RX_DROP;
            }

            if (rule_type == NAT_SRC) {
                ip->saddr = new_ip;
                tcp->source = (__be16)
                new_port;
                pcn_log(
                        ctx, LOG_TRACE, "NATted TCP pkt: (old_src: %I:%P) (new_src: %I:%P)",
                        old_ip, old_port, new_ip, new_port
                );
            } else {
                ip->daddr = new_ip;
                tcp->dest = (__be16)
                new_port;
                pcn_log(
                        ctx, LOG_TRACE, "NATted TCP pkt: (old_dst: %I:%P) (new_dst: %I:%P)",
                        old_ip, old_port, new_ip, new_port
                );
            }

            // Update checksums
            pcn_l4_csum_replace(ctx, TCP_CSUM_OFFSET, 0, l3sum, IS_PSEUDO | 0);
            pcn_l4_csum_replace(ctx, TCP_CSUM_OFFSET, 0, l4sum, 0);
            pcn_l3_csum_replace(ctx, IP_CSUM_OFFSET, 0, l3sum, 0);
            break;
        }
        case IPPROTO_UDP: {
            struct udphdr *udp = data + sizeof(*eth) + sizeof(*ip);
            if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*udp) > data_end) {
                pcn_log(ctx, LOG_TRACE,
                        "dropped UDP pkt after new checksum evaluation - (in_port: %d) (reason: inconsistent_size)",
                        md->in_port);
                return RX_DROP;
            }

            if (rule_type == NAT_SRC) {
                ip->saddr = new_ip;
                udp->source = (__be16)
                new_port;
                pcn_log(
                        ctx, LOG_TRACE, "NATted UDP pkt: (old_src: %I:%P) (new_src: %I:%P)",
                        old_ip, old_port, new_ip, new_port
                );
            } else {
                ip->daddr = new_ip;
                udp->dest = (__be16)
                new_port;
                pcn_log(
                        ctx, LOG_TRACE, "NATted UDP pkt: (old_dst: %I:%P) (new_dst: %I:%P)",
                        old_ip, old_port, new_ip, new_port
                );
            }

            // Update checksums
            pcn_l4_csum_replace(ctx, UDP_CSUM_OFFSET, 0, l3sum, IS_PSEUDO | 0);
            pcn_l4_csum_replace(ctx, UDP_CSUM_OFFSET, 0, l4sum, 0);
            pcn_l3_csum_replace(ctx, IP_CSUM_OFFSET, 0, l3sum, 0);
            break;
        }
        case IPPROTO_ICMP: {
            struct icmphdr *icmp = data + sizeof(*eth) + sizeof(*ip);
            if (data + sizeof(*eth) + sizeof(*ip) + sizeof(*icmp) > data_end) {
                pcn_log(ctx, LOG_TRACE,
                        "dropped ICMP pkt after new checksum evaluation - (in_port: %d) (reason: inconsistent_size)",
                        md->in_port);
                return RX_DROP;
            }

            if (rule_type == NAT_SRC) {
                ip->saddr = new_ip;
                icmp->un.echo.id = (__be16)
                new_port;
                pcn_log(
                        ctx, LOG_TRACE, "NATted ICMP pkt: (old_src: %I:%P) (new_src: %I:%P)",
                        old_ip, old_port, new_ip, new_port
                );
            } else {
                ip->daddr = new_ip;
                icmp->un.echo.id = (__be16)
                new_port;
                pcn_log(
                        ctx, LOG_TRACE, "NATted ICMP pkt: (old_dst: %I:%P) (new_dst: %I:%P)",
                        old_ip, old_port, new_ip, new_port
                );
            }

            // Update checksums
            pcn_l4_csum_replace(ctx, ICMP_CSUM_OFFSET, 0, l4sum, 0);
            pcn_l3_csum_replace(ctx, IP_CSUM_OFFSET, 0, l3sum, 0);
            break;
        }
    }

    // Session tables have been updated (if needed)
    // The packet has been modified (if needed)
    // Nothing more to do, forward the packet
    return pcn_pkt_redirect(ctx, md, md->in_port == BACKEND_PORT ? FRONTEND_PORT : BACKEND_PORT);


    ARP:;
    struct arp_hdr *arp = data + sizeof(*eth);
    if (data + sizeof(*eth) + sizeof(*arp) > data_end) {
        pcn_log(ctx, LOG_TRACE, "dropped ARP pkt - (in_port: %d) (reason: inconsistent_size)", md->in_port);
        return RX_DROP;
    }


    // Packet coming from the backend port
    if (md->in_port == BACKEND_PORT) {
        // If a packet coming from the backend port
        // - is an ARP request => it is sent to the frontend port
        // - otherwise it is dropped
        if (arp->ar_op == bpf_htons(ARPOP_REQUEST)) {
            pcn_log(
                    ctx, LOG_TRACE,
                    "received ARP request pkt from BACKEND port: redirected to FRONTEND port - (in_port: %d) (out_port: %d)",
                    md->in_port, FRONTEND_PORT);
            return pcn_pkt_redirect(ctx, md, FRONTEND_PORT);
        }
        pcn_log(
                ctx, LOG_TRACE, "dropped non-request ARP pkt coming from BACKEND port - (in_port: %d) (arp_opcode: %d)",
                md->in_port, bpf_htons(arp->ar_op)
        );
        return RX_DROP;
    }
    // Packet coming from the frontend port
    else {
        // If a packet coming from the frontend port
        // - is an ARP request => it is sent to the stack
        // - is an ARP reply => a copy is sent to the slowpath and the original one to the stack
        // - otherwise it is dropped
        if (arp->ar_op == bpf_ntohs(ARPOP_REQUEST)) {
            pcn_log(
                    ctx, LOG_TRACE,
                    "received ARP request pkt from FRONTEND port: sent to the stack - (in_port: %d)",
                    md->in_port
            );
            return RX_OK;
        }
        else if (arp->ar_op == bpf_htons(ARPOP_REPLY)) {
            pcn_log(
                    ctx, LOG_TRACE,
                    "received ARP reply pkt from FRONTEND port: sent a copy to slowpath and a copy to the stack - (in_port: %d)",
                    md->in_port
            );
            // a copy is sent to the backend port through the slowpath and a copy is sent to the stack
            pcn_pkt_controller(ctx, md, REASON_ARP_REPLY);
            return RX_OK;
        }
        pcn_log(
                ctx, LOG_TRACE, "dropped non-reply ARP pkt coming from FRONTEND port - (in_port: %d) (arp_opcode: %d)",
                md->in_port, bpf_htons(arp->ar_op)
        );
        return RX_DROP;
    }
}
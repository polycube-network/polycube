/*
 * Copyright 2017 The Polycube Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <bcc/helpers.h>
#include <bcc/proto.h>

#include <uapi/linux/bpf.h>
#include <uapi/linux/filter.h>
#include <uapi/linux/if_ether.h>
#include <uapi/linux/if_packet.h>
#include <uapi/linux/if_vlan.h>

#include <uapi/linux/in.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/pkt_cls.h>

enum { BPDU_REASON = 1, BROADCAST_REASON };

enum { PORT_MODE_ACCESS = 0, PORT_MODE_TRUNK };

#define VLAN_WILDCARD 0x00

enum stp_state {
  STP_DISABLED = 1 << 0,   /* 8.4.5: See note above. */
  STP_LISTENING = 1 << 1,  /* 8.4.2: Not learning or relaying frames. */
  STP_LEARNING = 1 << 2,   /* 8.4.3: Learning but not relaying frames. */
  STP_FORWARDING = 1 << 3, /* 8.4.4: Learning and relaying frames. */
  STP_BLOCKING = 1 << 4    /* 8.4.1: Initial boot state. */
};

/* Returns true if 'state' is one in which packets received on a port should
 * be forwarded, false otherwise.
 *
 * Returns true if 'state' is STP_DISABLED, since presumably in that case the
 * port should still work, just not have STP applied to it. */
static __always_inline u32 stp_forward_in_state(u32 state) {
  return (state & (STP_DISABLED | STP_FORWARDING)) != 0;
}

/* Returns true if 'state' is one in which MAC learning should be done on
 * packets received on a port, false otherwise.
 *
 * Returns true if 'state' is STP_DISABLED, since presumably in that case the
 * port should still work, just not have STP applied to it. */
static __always_inline u32 stp_learn_in_state(u32 state) {
  return (state & (STP_DISABLED | STP_LEARNING | STP_FORWARDING)) != 0;
}

/*
 * Allowed Vlans table is used to set and verify vlans associated to a port
 * in access or trunk mode. The usage is provided in the following example.
*/
struct port_vlan_key {
  // todo use u16 instead of u32
  u32 port;
  u32 vlan;
} __attribute__((packed));

struct port_vlan_value {
  u16 vlan;
  u16 stp_state;
} __attribute__((packed));

BPF_TABLE("hash", struct port_vlan_key, struct port_vlan_value, port_vlan,
          1024);
/*
 * port_vlan usage example:
 * <port> <vlan> -> vlanid
 *
 * port 1 access vlan=10
 * 1 VLAN_WILDCARD -> 10
 *
 * port 3 trunk vlan=30,40
 * 3 30 -> 30
 * 3 40 -> 40
 *
 * port 4 trunk vlan=all
 * 4 VLAN_WILDCARD -> VLAN_WILDCARD
*/

/*
 * Fowarding Table: VLAN + MAC Address to port association.  This table is
 * filled
 * in the learning phase and then is used in the forwarding phase to decide
 * where to send a packet
*/
struct fwd_key {
  u32 vlan;
  __be64 mac;
} __attribute__((packed));

struct fwd_entry {
  u64 timestamp;
  u32 port;
} __attribute__((packed));

BPF_TABLE("hash", struct fwd_key, struct fwd_entry, fwdtable, 1024);

/*
 * Port Table: Saves the state of the ports
 * TODO: what is the best option in this case, hash vs array?
*/
struct port {
  u16 mode;
  u16 native_vlan;
};

BPF_TABLE("hash", u32, struct port, ports, 256);

struct eth_hdr {
  __be64 dst : 48;
  __be64 src : 48;
  __be16 proto;
} __attribute__((packed));

static __always_inline int handle_rx(struct CTXTYPE *ctx,
                                     struct pkt_metadata *md) {
  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;
  struct eth_hdr *eth = data;

  if (data + sizeof(*eth) > data_end)
    goto DROP;

  u32 in_ifc = md->in_port;

  pcn_log(ctx, LOG_DEBUG, "in_ifc=%d proto=0x%x mac src:%M dst:%M", in_ifc,
          bpf_ntohs(eth->proto), eth->src, eth->dst);

  u32 vlanid = 0;
#ifdef POLYCUBE_XDP
  bool tagged = pcn_is_vlan_present(ctx);

  if (tagged) {
    u16 vlan_u16;
    u16 eth_proto;
    if (pcn_get_vlan_id(ctx, &vlan_u16, &eth_proto) < 0)
      goto DROP;

    vlanid = (u32)vlan_u16;
  }
#else
  bool tagged = ctx->vlan_present;

  if (tagged)
    vlanid = ctx->vlan_tci & 0x0fff;
#endif

  struct port *in_port = ports.lookup(&in_ifc);
  if (!in_port) {
    pcn_log(ctx, LOG_ERR, "Port %u not found", in_ifc);
    goto DROP;
  }

  struct port_vlan_key vlan_key;
  struct port_vlan_value *vlan_entry;

  vlan_key.port = in_ifc;

  if (in_port->mode == PORT_MODE_ACCESS) {
    if (tagged)
      goto DROP;

    vlan_key.vlan = VLAN_WILDCARD;
    vlan_entry = port_vlan.lookup(&vlan_key);
    if (!vlan_entry)
      goto DROP;  // TODO: is this code correct?

    vlanid = vlan_entry->vlan;
    pcn_log(ctx, LOG_TRACE, "port mode ACCESS vlan=%d, state=%d", vlanid,
            vlan_entry->stp_state);
  } else {
    // TRUNK
    if (!tagged) {
      vlanid = in_port->native_vlan;
    } else {
      vlan_key.vlan = vlanid;
      vlan_entry = port_vlan.lookup(&vlan_key);
      if (!vlan_entry) {
        // TODO manage case of all vlans allowed
        goto DROP;
      }
    }
  }

  // TODO: this is probably wrong or could done better, for example adding
  // a fake entry on the forwarding table
  // if the packet is a BPDU, send it to the slowpath
  if (eth->dst == 0x000000C28001 || eth->dst == 0xCDCCCC0C0001) {
    u32 mdata[3];
    mdata[0] = vlanid;
    mdata[1] = tagged;
    return pcn_pkt_controller_with_metadata(ctx, md, BPDU_REASON, mdata);
  }

  // Should learning be performed?
  if (!stp_learn_in_state(vlan_entry->stp_state)) {
    pcn_log(ctx, LOG_TRACE, "port %d should not learn: state: %d", in_ifc,
            vlan_entry->stp_state);
    return RX_DROP;
  }

  // LEARNING PHASE
  struct fwd_key src_key;
  src_key.vlan = vlanid;
  src_key.mac = eth->src;
  struct fwd_entry e;  // used below
  e.timestamp = bpf_ktime_get_ns();
  e.port = in_ifc;
  fwdtable.update(&src_key, &e);

  // should forwarding be performed?
  if (!stp_forward_in_state(vlan_entry->stp_state)) {
    pcn_log(ctx, LOG_TRACE, "port %d should not forward\n", in_ifc);
    return RX_DROP;
  }

  // FORWARDING PHASE: select interface(s) to send the packet
  struct fwd_key dst_key;
  dst_key.vlan = vlanid;
  dst_key.mac = eth->dst;

  // lookup in forwarding table fwdtable
  struct fwd_entry *entry = fwdtable.lookup(&dst_key);
  if (!entry) {
    // bpf_trace_printk("entry not found\n");
    goto DO_FLOODING;
  }

  // check if the entry is too old
  u64 now = bpf_ktime_get_ns();
  u64 timestamp = entry->timestamp;
  if (now < timestamp) {
    goto FORWARD;
  }
  if (now - timestamp > AGING_TIME * 1000000000ULL) {
    // bpf_trace_printk("entry too old\n");
    fwdtable.delete(&dst_key);
    goto DO_FLOODING;
  }
  entry->timestamp = now;

FORWARD:;
  u32 dst_interface = entry->port;  // workaround for verifier
  struct port *out_port = ports.lookup(&dst_interface);
  if (!out_port) {
    pcn_log(ctx, LOG_ERR, "Port %u not found", dst_interface);
    goto DROP;
  }

  // HIT in forwarding table
  // redirect packet to dst_interface

  /* do not send packet back on the ingress interface */
  if (dst_interface == in_ifc)
    goto DROP;

  // check output vlan
  if (out_port->mode == PORT_MODE_ACCESS) {
    vlan_key.port = dst_interface;
    vlan_key.vlan = VLAN_WILDCARD;
    vlan_entry = port_vlan.lookup(&vlan_key);
    if (!vlan_entry) {
      // bpf_trace_printk("dropping paket: no entry in allowed vlans\n");
      goto DROP;
    }

    if (vlan_entry->vlan != vlanid) {
      // bpf_trace_printk("dropping packet: vlan_entry %d vlanid %d\n",
      // vlan_entry->vlan, vlanid);
      goto DROP;
    }

    if (tagged) {
#ifdef POLYCUBE_XDP
      if (pcn_pop_vlan_tag(ctx) != 0) {
        pcn_log(ctx, LOG_ERR, "error pcn_push_vlan_tag");
        goto DROP;
      }
#else
      uint8_t r = bpf_skb_vlan_pop(ctx);
      if (r)
        pcn_log(ctx, LOG_ERR, "error vlan_pop");
#endif
    }
  } else if (out_port->mode == PORT_MODE_TRUNK) {
    if (vlanid == out_port->native_vlan) {
      if (tagged) {
#ifdef POLYCUBE_XDP
        if (pcn_pop_vlan_tag(ctx) != 0) {
          pcn_log(ctx, LOG_ERR, "error pcn_push_vlan_tag");
          goto DROP;
        }
#else
        uint8_t r = bpf_skb_vlan_pop(ctx);
        if (r)
          pcn_log(ctx, LOG_ERR, "error vlan_pop");
#endif
      }
    } else {
      vlan_key.port = dst_interface;
      vlan_key.vlan = vlanid;
      vlan_entry = port_vlan.lookup(&vlan_key);
      if (!vlan_entry) {
        // bpf_trace_printk("dropping packet: trunk vlan entry not present\n");
        goto DROP;
      }
      if (!tagged) {
// todo check tag correctness
#ifdef POLYCUBE_XDP
        if (pcn_push_vlan_tag(ctx, eth->proto, vlanid) != 0) {
          goto DROP;
        }
#else
        uint8_t r = bpf_skb_vlan_push(ctx, eth->proto, vlanid);
        if (r)
          pcn_log(ctx, LOG_ERR, "error vlan_push");
#endif
      }
    }
  }

  if (!stp_forward_in_state(vlan_entry->stp_state)) {
    pcn_log(ctx, LOG_TRACE, "port %d should not forward\n", dst_interface);
    return RX_DROP;
  }

  pcn_log(ctx, LOG_TRACE, "redirect out_ifc=%d", dst_interface);
  return pcn_pkt_redirect(ctx, md, dst_interface);

DO_FLOODING:;
  pcn_log(ctx, LOG_TRACE, "broadcast");
  u32 mdata[3];
  mdata[0] = vlanid;
  mdata[1] = tagged;
  return pcn_pkt_controller_with_metadata(ctx, md, BROADCAST_REASON, mdata);

DROP:
  pcn_log(ctx, LOG_TRACE, "drop");
  return RX_DROP;
}

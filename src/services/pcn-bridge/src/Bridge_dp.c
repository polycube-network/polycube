/*
 * Copyright 2019 The Polycube Authors
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

enum entry_type { STATIC = 0, DYNAMIC };

#define VLAN_WILDCARD 0x00

#if STP_ENABLED
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
#endif

/*
 * Allowed Vlans table is used to set and verify vlans associated to a port
 * in access or trunk mode. The usage is provided in the following example.
 */
struct port_vlan_key {
  u32 port;
  u16 vlan;
} __attribute__((packed));

struct port_vlan_value {
  u16 vlan;
  u16 stp_state;
} __attribute__((packed));

BPF_TABLE("hash", struct port_vlan_key, struct port_vlan_value, port_vlan,
          1024);
/*
 * We can check if a vlan V is allowed for a certain port P if the value for
 * the key {P,V} is present in the table. If it's not present, the vlan V
 * is not allowed for the port P. Moreover, if the vlan is allowed, we can
 * also check the stp state of the port for that vlan (if stp is enabled)
 *
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
  u16 vlan;
  __be64 mac;
} __attribute__((packed));

struct fwd_entry {
  u32 timestamp;
  u32 port;
  enum entry_type type;
} __attribute__((packed));

BPF_TABLE("hash", struct fwd_key, struct fwd_entry, fwdtable, 1024);
BPF_TABLE("array", int, uint32_t, timestamp, 1);

static __always_inline u32 time_get_sec() {
  int key = 0;
  u32 *ts = timestamp.lookup(&key);
  if (ts)
    return *ts;

  return 0;
}

/*
 * Port Table: Saves the state of the ports
 * TODO: what is the best option in this case, hash vs array?
 */
struct port {
  u16 mode;
  u16 native_vlan;
  bool native_vlan_enabled;
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

  pcn_log(ctx, LOG_DEBUG, "in_ifc=%d proto=0x%x", in_ifc,
          bpf_ntohs(eth->proto));
  pcn_log(ctx, LOG_DEBUG, "mac src:%M dst:%M", eth->src, eth->dst);

  u16 vlanid = 0;

  bool tagged = pcn_is_vlan_present(ctx);

  if (tagged) {
    int result;
    if ((result = pcn_get_vlan_id(ctx)) < 0)
      goto DROP;
    vlanid = result;
  }

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
    if (!vlan_entry) {
      pcn_log(ctx, LOG_ERR, "no VLAN configured in this access port");
      goto DROP;
    }

    vlanid = vlan_entry->vlan;

    pcn_log(ctx, LOG_TRACE, "port mode ACCESS vlan=%d, state=%d", vlanid,
            vlan_entry->stp_state);
  } else {
    // TRUNK
    if (!tagged) {
      if (!in_port->native_vlan_enabled) {
        pcn_log(ctx, LOG_ERR,
                "Packet is not tagged but the port has native vlan disabled");
        goto DROP;
      }
      vlanid = in_port->native_vlan;
    }
    vlan_key.vlan = vlanid;
    vlan_entry = port_vlan.lookup(&vlan_key);
    if (!vlan_entry) {
      // TODO manage case of all vlans allowed
      pcn_log(ctx, LOG_ERR,
              "VLAN of the packet is not configured in this trunk port");
      goto DROP;
    }
  }

#if STP_ENABLED
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
#endif

  u32 now = time_get_sec();

  // LEARNING PHASE
  struct fwd_key src_key;
  src_key.vlan = vlanid;
  src_key.mac = eth->src;
  struct fwd_entry e;  // used below
  e.timestamp = now;
  e.port = in_ifc;
  e.type = DYNAMIC;

  // make sure that not exist a static entry associated to this address & vlan
  struct fwd_entry *src_entry = fwdtable.lookup(&src_key);
  if (!src_entry || src_entry->type == DYNAMIC) {
    fwdtable.update(&src_key, &e);
  }

#if STP_ENABLED
  // should forwarding be performed?
  if (!stp_forward_in_state(vlan_entry->stp_state)) {
    pcn_log(ctx, LOG_TRACE, "port %d should not forward", in_ifc);
    return RX_DROP;
  }
#endif

  // FORWARDING PHASE: select interface(s) to send the packet
  struct fwd_key dst_key;
  dst_key.vlan = vlanid;
  dst_key.mac = eth->dst;

  // lookup in forwarding table fwdtable
  struct fwd_entry *entry = fwdtable.lookup(&dst_key);
  if (!entry) {
    pcn_log(ctx, LOG_TRACE, "entry not found in filtering database, flooding");
    goto DO_FLOODING;
  }

  if (entry->type == STATIC) {
    goto FORWARD;
  }

  // check if the entry is too old
  u32 timestamp = entry->timestamp;
  if (now - timestamp > AGING_TIME) {
    pcn_log(ctx, LOG_DEBUG, "Entry too old");
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
      goto DROP;
    }

    if (vlan_entry->vlan != vlanid) {
      goto DROP;
    }

    if (tagged) {
      if (pcn_vlan_pop_tag(ctx) != 0) {
        pcn_log(ctx, LOG_ERR, "error pcn_vlan_pop_tag");
        goto DROP;
      }
    }
  } else if (out_port->mode == PORT_MODE_TRUNK) {
    vlan_key.port = dst_interface;
    vlan_key.vlan = vlanid;
    vlan_entry = port_vlan.lookup(&vlan_key);
    if (!vlan_entry) {
      goto DROP;
    }
    if (out_port->native_vlan_enabled && vlanid == out_port->native_vlan) {
      if (tagged) {
        if (pcn_vlan_pop_tag(ctx) != 0) {
          pcn_log(ctx, LOG_ERR, "error pcn_vlan_pop_tag");
          goto DROP;
        }
      }
    } else {
      if (!tagged) {
        if (pcn_vlan_push_tag(ctx, eth->proto, vlanid) != 0) {
          pcn_log(ctx, LOG_ERR, "error pcn_vlan_push_tag");
          goto DROP;
        }
      }
    }
  }

#if STP_ENABLED
  if (!stp_forward_in_state(vlan_entry->stp_state)) {
    pcn_log(ctx, LOG_TRACE, "port %d should not forward\n", dst_interface);
    return RX_DROP;
  }
#endif

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

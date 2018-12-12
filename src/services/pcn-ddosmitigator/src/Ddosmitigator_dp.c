/*
 * Copyright 2018 The Polycube Authors
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


#define SRC_MATCH _SRC_MATCH
#define DST_MATCH _DST_MATCH

#define IN_PORT _IN_PORT
// out port is used in redirect mode
#define OUT_PORT _OUT_PORT

// if redirect is defined ddos mitigator works as a passtrough.
// else traffic not dropped is passed to stack
#define REDIRECT _REDIRECT

#include <uapi/linux/bpf.h>
#include <uapi/linux/in.h>
#include <uapi/linux/if_ether.h>
#include <uapi/linux/if_packet.h>
#include <uapi/linux/if_vlan.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/ipv6.h>

/*
 * dropcount is used to store dropped pkts counters.
 * key (uint32_t): [0] always stored at same array position
 * value (u64): pkts counter.
 */
BPF_TABLE("percpu_array", int, u64, dropcnt, 1);

/*
 * srcblacklist is used to lookup and filter pkts using ipv4 src addresses.
 * key (uint32_t): ipv4 address.
 * value (u64): used for matched rules counters.
 */
#if SRC_MATCH
BPF_TABLE("percpu_hash", uint32_t, u64, srcblacklist, 1024);
//TODO it should be u64 as value
#endif

/*
 * dstblacklist is used to lookup and filter pkts using ipv4 dst addresses.
 * key (uint32_t): ipv4 address.
 * value (u64): used for matched rules counters.
 */
#if DST_MATCH
BPF_TABLE("percpu_hash", uint32_t, u64, dstblacklist, 1024);
//TODO it should be u64 as value
#endif

/*
 * This function is called each time a packet arrives to the cube.
 * ctx contains the packet and md some additional metadata for the packet.
 * If the service is of type XDP_SKB/DRV CTXTYPE is equivalent to the struct xdp_md
 * otherwise, if the service is of type TC, CTXTYPE is equivalent to the __sk_buff struct
 * Please look at the libpolycube documentation for more details.
 */
static inline int parse_ipv4(void *data, u64 nh_off, void *data_end) {
    struct iphdr *iph = data + nh_off;

    if ((void*)&iph[1] > data_end)
        return 0;

  #if SRC_MATCH
    uint32_t src = iph->saddr;

    u64 *cntsrc = srcblacklist.lookup(&src);
    if (cntsrc){
        *cntsrc += 1;
        return iph->protocol;
    }

  #endif

  #if DST_MATCH
    uint32_t dst = iph->daddr;

    u64 *cntdst = dstblacklist.lookup(&dst);
    if (cntdst){
        *cntdst += 1;
        return iph->protocol;
    }

  #endif

    return 0;
}

static __always_inline
int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
    void* data_end = (void*)(long)ctx->data_end;
    void* data = (void*)(long)ctx->data;
    struct ethhdr *eth = data;

  #if REDIRECT
    if (md->in_port == OUT_PORT){
      pcn_log(ctx, LOG_DEBUG, "Redirect packet OUT_PORT -> IN_PORT %d -> %d ", OUT_PORT, IN_PORT );
      return pcn_pkt_redirect(ctx, md, IN_PORT);
    }
  #endif

    // if redirect mode is disabled, md->in_port should be IN_PORT for all packets
    // no need to check it if control plane guarantees only one interface attached.

    uint64_t offset = 0;
    int result = 0;
    uint16_t ethtype;
    u64 *value;

    offset = sizeof(*eth);

    if (data + offset > data_end)
        goto PASS;

    ethtype = eth->h_proto;
    if (ethtype == htons(ETH_P_IP))
        result = parse_ipv4(data, offset, data_end);

    if (result == 0){
      goto PASS;
    }

    u32 index = 0;
    value = dropcnt.lookup(&index);
    if (value){
        *value += 1;
        pcn_log(ctx, LOG_DEBUG, "Dropcount proto:%d value: %d ", result, *value);
    }

    pcn_log(ctx, LOG_DEBUG, "Dropping packet ethtype: %x ", eth->h_proto);
    return RX_DROP;

   PASS:
   #if REDIRECT
      pcn_log(ctx, LOG_DEBUG, "Redirect packet IN_PORT -> OUT_PORT %d -> %d ", IN_PORT, OUT_PORT );
      return pcn_pkt_redirect(ctx, md, OUT_PORT);
   #else
    pcn_log(ctx, LOG_DEBUG, "Passing packet to Stack ");
    return RX_OK;
   #endif
}

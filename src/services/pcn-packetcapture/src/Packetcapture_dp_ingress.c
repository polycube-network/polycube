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


/*
 * This function is called each time a packet arrives to the cube.
 * ctx contains the packet and md some additional metadata for the packet.
 * If the service is of type XDP_SKB/DRV CTX TYPE is equivalent to the struct
 * xdp_md otherwise, if the service is of type TC, CTXTYPE is equivalent to
 * the __sk_buff struct
 * Please look at the libpolycube documentation for more details.
 */


#include <bcc/helpers.h>
#include <uapi/linux/if_ether.h>
#include <uapi/linux/in.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/tcp.h>
#include <uapi/linux/udp.h>

struct eth_hdr {
    __be64 dst : 48;
    __be64 src : 48;
    __be16 proto;
} __attribute__((packed));

/*
 * BPF map where a single element, packet timestamp
 */
BPF_ARRAY(packet_timestamp, uint64_t, 1);


static __always_inline int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {
    unsigned int key = 0;
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    struct eth_hdr *ethernet = data;
    u16 reason = 1;
    uint32_t a, x, m[16];

    uint64_t *pkt_timestamp = packet_timestamp.lookup(&key);
    if (!pkt_timestamp){
        return RX_DROP;
    }
    *pkt_timestamp = bpf_ktime_get_ns();

    //CUSTOM_FILTER_CODE

    return RX_OK;

}
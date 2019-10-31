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

 struct filters_table {
  bool network_filter_src_flag;
  uint32_t network_filter_src;
  uint32_t netmask_filter_src;
  bool network_filter_dst_flag;
  uint32_t network_filter_dst;
  uint32_t netmask_filter_dst;
  bool src_port_flag;
  uint16_t src_port_filter;
  bool dst_port_flag;
  uint16_t dst_port_filter;
  bool l4proto_flag;
  int l4proto_filter;   /* if l4proto_filter = 1 is TCP only else if l4proto_filter = 2 is UDP only */
  uint32_t snaplen;
};

enum {
  ON,
  OFF,
};

struct eth_hdr {
  __be64 dst : 48;
  __be64 src : 48;
  __be16 proto;
} __attribute__((packed));


/*
 * BPF map where a single element, packet timestamp
 */
BPF_ARRAY(packet_timestamp, uint64_t, 1);

/*
 * BPF map where a single element, working mode [ON | OFF]
 */
BPF_ARRAY(working, uint8_t, 1);

/*
 * BPF map where a single element, the filters map
 */
BPF_ARRAY(filters_map, struct filters_table, 1);


static __always_inline int handle_rx(struct CTXTYPE *ctx, struct pkt_metadata *md) {

  unsigned int key = 0;
  uint8_t *status = working.lookup(&key);
  if (!status){
      return RX_DROP;
  }
  
  if (*status == OFF){
      return RX_OK;
  }
  
  /* Parsing L2 */
  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;
  struct eth_hdr *ethernet = data;
  if (data + sizeof(*ethernet) > data_end)
    return RX_DROP;

  uint16_t ether_type = ethernet->proto;

  if (ctx->vlan_present) {
    ether_type = ctx->vlan_proto;
  }

  /* Parsing L3 */
  struct iphdr *ip = NULL;
  struct tcphdr *tcp = NULL;
  struct udphdr *udp = NULL;
  
  if (ether_type == bpf_htons(ETH_P_IP)) {
    ip = data + sizeof(*ethernet);  
    if (data + sizeof(*ethernet) + sizeof(*ip) > data_end)
      return RX_DROP;   
    if ((int)ip->version != 4){
      return RX_OK;
    }

    struct filters_table *filters_tab;
    filters_tab = filters_map.lookup(&key);
    if (filters_tab == NULL) {
      return RX_DROP;
    }
    
    /* Source IP filter */
    if ((filters_tab->network_filter_src_flag == true) && ((filters_tab->netmask_filter_src & ntohl(ip->saddr)) != filters_tab->network_filter_src)){
      return RX_OK;
    }
      
    /* Destination IP filter */
    if ((filters_tab->network_filter_dst_flag == true) && ((filters_tab->netmask_filter_dst & ntohl(ip->daddr)) != filters_tab->network_filter_dst)){
      return RX_OK;
    }

    /* l4proto Filter */
    if (filters_tab->l4proto_flag == true){
      if ((filters_tab->l4proto_filter == 1) && (ip->protocol != IPPROTO_TCP)){         /* tcp only */
        return RX_OK;
      }else if ((filters_tab->l4proto_filter == 2) && (ip->protocol != IPPROTO_UDP)){   /* udp only */
        return RX_OK;
      }
    }

    uint8_t header_len = 4 * ip->ihl; 
    if (ip->protocol == IPPROTO_TCP) {
      tcp = data + sizeof(*ethernet) + header_len;
      if (data + sizeof(*ethernet) + header_len + sizeof(*tcp) > data_end)
        return RX_DROP;
      
      /* src port filter */
      if ((filters_tab->src_port_flag == true) && (filters_tab->src_port_filter != ntohs(tcp->source))){
        return RX_OK;
      }

      /* dst port filter */
      if ((filters_tab->dst_port_flag == true) && (filters_tab->dst_port_filter != ntohs(tcp->dest))){
        return RX_OK;
      }
    } else if (ip->protocol == IPPROTO_UDP) {
      udp = data + sizeof(*ethernet) + header_len;
      if (data + sizeof(*ethernet) + header_len + sizeof(*udp) > data_end)
        return RX_DROP;
    }

  }
  
  /*                Getting packet timestamp
   *
   * see line 58 in Packetcapture.cpp for more details about timestamping algorithm
   */
  uint64_t *pkt_timestamp = packet_timestamp.lookup(&key);
  if (!pkt_timestamp){
      return RX_DROP;
  }
  *pkt_timestamp = bpf_ktime_get_ns();
  
  /*TODO: Now the traffic will be sent to the controller and will be processed in the slow path.
   *      This solution will have a big impact on performance.
   *      Currently, non-IP traffic is also sent to the controller, if this type of traffic is to be excluded
   *      from capture, a map should be inserted indicating the decision taken at the fast path.
   *      
   * 
   *      Example:
   *      
   *      if ( only_IP_flag && ip != NULL) {
   *        pass the packet to controller
   *      } else {
   *        NOP
   *      }
   * 
   *      To optimize performance we can try to copy the packet and only send a copy of it to the slow path but currently
   *      this feature is not fully functional in polycube.
   *      see "pcn_pkt_controller_with_metadata_stack()"
   */
  
  u16 reason = 1;
  return pcn_pkt_controller(ctx, md, reason);

}

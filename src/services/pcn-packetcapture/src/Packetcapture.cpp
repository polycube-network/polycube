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


#include <string>
#include <sys/time.h>
#include <iomanip>
#include <iostream>
#include <ctime>
#include <fstream>
#include <chrono>
#include "unistd.h"
#include "Packetcapture.h"
#include "Packetcapture_dp_ingress.h"
#include "Packetcapture_dp_egress.h"
#define ON_T 0
#define OFF_T 1
#define BILLION 1000000000

typedef int bpf_int32; 
typedef u_int bpf_u_int32;

struct pcap_file_header {
  bpf_u_int32 magic;      /* magic number */
  u_short version_major;  /* major version number */
  u_short version_minor;  /* minor version number */
  bpf_int32 thiszone;     /* GMT to local correction */
  bpf_u_int32 sigfigs;    /* accuracy of timestamps */
  bpf_u_int32 snaplen;    /* max length of captured packets, in octets */
  bpf_u_int32 linktype;   /* data link type */
};

struct pcap_pkthdr {
  bpf_u_int32 ts_sec;
  bpf_u_int32 ts_usec;  
  bpf_u_int32 caplen;     /* number of octets of packet saved in file */
  bpf_u_int32 len;        /* actual length of packet */
};

Packetcapture::Packetcapture(const std::string name, const PacketcaptureJsonObject &conf)
  : TransparentCube(conf.getBase(), { packetcapture_code_ingress }, { packetcapture_code_egress }),
    PacketcaptureBase(name) {
  
  /*
   * The timestamp of the packet is acquired in the fast path using 'bpf_ktime_get_ns()'
   * This function returns the time elapsed since system booted, in nanoseconds.
   * See line 173 in Packetcapture_dp_egress.c and Packetcapture_dp_ingress.c
   * 
   * The packet's timestamp must be in epoch so now we will store the current
   * time in epoch format and then the packet capture time offset,that was stored in the fast path,
   * will be added to it.
   * 
   * Now we are calculating the system start time in epoch.
   * 'timeval struct ts' rapresents the system start time in epoch calculated as
   * actual time - system uptime
   * 
   * See line 161 or 329 of this file for more details
   */


  timeP = std::chrono::high_resolution_clock::now();  //getting actual time

  std::chrono::nanoseconds uptime(0u);
  double uptime_seconds;
  if (std::ifstream("/proc/uptime", std::ios::in) >> uptime_seconds)
  {
    uptime = std::chrono::nanoseconds(
      static_cast<unsigned long long>(uptime_seconds*BILLION)
    );
  }

  timeP -= uptime;

  logger()->info("Creating Packetcapture instance");
    setCapture(conf.getCapture());
    setAnomimize(conf.getAnomimize());

  if (conf.dumpIsSet()) {
    setDump(conf.getDump());
  }

  setNetworkmode(conf.getNetworkmode());
  addFilters(conf.getFilters());
  addGlobalheader(conf.getGlobalheader());
  CapStatus = (uint8_t) conf.getCapture();

  auto t_filters_in = get_array_table<filters_table>("filters_map", 0, ProgramType::INGRESS);
  auto t_filters_out = get_array_table<filters_table>("filters_map", 0, ProgramType::EGRESS);
  filters_table ft_init;
  ft_init.dst_port_flag = ft_init.src_port_flag = ft_init.network_filter_src_flag = ft_init.network_filter_dst_flag = ft_init.l4proto_flag = false;
  t_filters_in.set(0x0, ft_init);
  t_filters_out.set(0x0, ft_init);
  writeHeader = true;
  dt = "";
}

void Packetcapture::writeDump(const std::vector<uint8_t> &packet){

  PacketJsonObject pj;
  auto p = std::shared_ptr<Packet>(new Packet(*this, pj));
  std::streamsize len;

  if (dt == "") {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    std::stringstream tmstmp;
    tmstmp << ltm->tm_mday << "-" << ltm->tm_mon << "-" << (ltm->tm_year + 1900) << "-" << ltm->tm_hour << ":" << ltm->tm_min << ":" << ltm->tm_sec;
    dt = tmstmp.str();

    //getting temp folder
    //std::filesystem::temp_directory_path only in c++17
    //according to ISO/IEC 9945 (POSIX)
    char const *folder = getenv("TMPDIR");
    if (folder == 0){
      folder = getenv("TMP");
      if (folder == 0){
        folder = getenv("TEMP");
        if (folder == 0){
          folder = getenv("TEMPDIR");
          if (folder == 0){
            folder = "/tmp";
          }
        }
      }
    }
    temp_folder = std::string(folder);
    temp_folder.append("/");
    random_number = std::to_string(rand()%1000);    //to avoid two files that using the same name
  }
  
  myFile.open(temp_folder + "capture_" + dt + "_" + random_number + ".pcap", std::ios::binary | std::ios::app);

  if (writeHeader == true) {
    struct pcap_file_header *pcap_header = new struct pcap_file_header;
    pcap_header->magic = global_header->getMagic();
    pcap_header->version_major = global_header->getVersionMajor();
    pcap_header->version_minor = global_header->getVersionMinor();
    pcap_header->thiszone = global_header->getThiszone();   //timestamp are always in GMT
    pcap_header->sigfigs = global_header->getSigfigs();
    pcap_header->snaplen = filters->getSnaplen();
    pcap_header->linktype = global_header->getLinktype();

    myFile.write(reinterpret_cast<const char*>(pcap_header), sizeof(*pcap_header));
    writeHeader = false;
  }

  /*
   * Here the packet capture time offset must be added to the system boot time stored in epoch format.
   * See line 58 of this file to see system boot time stored in epoch algorithm
   */

  struct timeval tp;
  std::chrono::system_clock::duration tp_dur = (timeP + temp_offset).time_since_epoch();
  to_timeval(tp_dur, tp);
  
  p->setTimestampSeconds((uint32_t) tp.tv_sec);
  p->setTimestampMicroseconds((uint32_t) tp.tv_usec);
  p->setPacketlen((uint32_t) packet.size());
  if (p->getPacketlen() > filters->getSnaplen()) {
    p->setCapturelen(filters->getSnaplen());
  } else {
    p->setCapturelen(p->getPacketlen());
  }
  p->setRawPacketData(packet);

  struct pcap_pkthdr *pkt_hdr = new struct pcap_pkthdr;
  pkt_hdr->ts_sec = p->getTimestampSeconds();
  pkt_hdr->ts_usec = p->getTimestampMicroseconds();
  pkt_hdr->len = p->getPacketlen();
  pkt_hdr->caplen = p->getCapturelen();
  filters->getSnaplen() < p->getRawPacketData().size() ? len = filters->getSnaplen() : len = p->getRawPacketData().size();

  myFile.write(reinterpret_cast<const char*>(pkt_hdr), sizeof(*pkt_hdr));
  myFile.write(reinterpret_cast<const char*>(&p->getRawPacketData()[0]), len );
  myFile.close();
}

Packetcapture::~Packetcapture() {
  logger()->info("Destroying Packetcapture instance");
}

void Packetcapture::packet_in(polycube::service::Direction direction,
    polycube::service::PacketInMetadata &md,
    const std::vector<uint8_t> &packet) {
  
  Tins::EthernetII pkt(&packet[0], packet.size());

  switch (direction) {
    case polycube::service::Direction::INGRESS:
    temp_offset = std::chrono::nanoseconds(static_cast<unsigned long long>(get_array_table<uint64_t>("packet_timestamp", 0, ProgramType::INGRESS).get(0x0)));
    if (getNetworkmode() == true) {
      addPacket(packet);    /* store the packet in the FIFO queue*/
    } else {
      writeDump(packet);
    }
    break;
    case polycube::service::Direction::EGRESS:
    temp_offset = std::chrono::nanoseconds(static_cast<unsigned long long>(get_array_table<uint64_t>("packet_timestamp", 0, ProgramType::EGRESS).get(0x0)));
    if (getNetworkmode() == true) {
      addPacket(packet);    /* store the packet in the FIFO queue*/
    } else {
      writeDump(packet);
    }
    break;
  }
  send_packet_out(pkt, direction, false);
}

PacketcaptureCaptureEnum Packetcapture::getCapture() {
  return static_cast<PacketcaptureCaptureEnum>(CapStatus);
}

void Packetcapture::setCapture(const PacketcaptureCaptureEnum &value) {

  auto t_in = get_array_table<uint8_t>("working", 0, ProgramType::INGRESS);
  auto t_out = get_array_table<uint8_t>("working", 0, ProgramType::EGRESS);

  switch (value) {
    case PacketcaptureCaptureEnum::INGRESS:
      t_in.set(0x0, ON_T);
      t_out.set(0x0, OFF_T);
      CapStatus = 0;
    break;
    case PacketcaptureCaptureEnum::EGRESS:
      t_in.set(0x0, OFF_T);
      t_out.set(0x0, ON_T);
      CapStatus = 1;
    break;
    case PacketcaptureCaptureEnum::BIDIRECTIONAL:
      t_in.set(0x0, ON_T);
      t_out.set(0x0, ON_T);
      CapStatus = 2;
    break;
    case PacketcaptureCaptureEnum::OFF:
      t_in.set(0x0, OFF_T);
      t_out.set(0x0, OFF_T);
      CapStatus = 3;
    break;
  }
}

bool Packetcapture::getAnomimize() {
  //TODO
}

void Packetcapture::setAnomimize(const bool &value) {
  //TODO
}

std::string Packetcapture::getDump() {
  std::stringstream dump;

  if (network_mode_flag) {
    dump << "the service is running in network mode";
  } else if (temp_folder != ""){
      char *cwdr_ptr = get_current_dir_name();
      std::string wdr(cwdr_ptr);
      dump << "capture dump in " << temp_folder << "capture_" + dt + "_" + random_number + ".pcap" << std::endl;
  } else{
    dump << "no packets captured";
  }
  
  return dump.str();
}

void Packetcapture::setDump(const std::string &value) {
    throw std::runtime_error("Packetcapture::setDump: Method not implemented");
}

bool Packetcapture::getNetworkmode() {
  return network_mode_flag;
}

void Packetcapture::setNetworkmode(const bool &value) {
  network_mode_flag = value;
}

std::shared_ptr<Filters> Packetcapture::getFilters() {
  return filters;
}

void Packetcapture::addFilters(const FiltersJsonObject &value) {
  filters = std::shared_ptr<Filters>(new Filters(*this, value));
}

void Packetcapture::replaceFilters(const FiltersJsonObject &conf) {
  PacketcaptureBase::replaceFilters(conf);
}

void Packetcapture::delFilters() {
  throw std::runtime_error("Packetcapture::delFilters: method not implemented");
}

/* return the first packet in the queue if exist */
std::shared_ptr<Packet> Packetcapture::getPacket() {
  if (packets_captured.size() != 0) {
    auto p = packets_captured.front();
    if (network_mode_flag) {                            /* pop this element */
      packets_captured.erase(packets_captured.begin());
      if (filters->getSnaplen() < p->getPacketlen()) {
        p->packet.resize(filters->getSnaplen());
      }
    }
    return p;
  }
  PacketJsonObject pj;
  auto p = std::shared_ptr<Packet>(new Packet(*this, pj));
  return p;
}

void Packetcapture::addPacket(const std::vector<uint8_t> &packet) {
    PacketJsonObject pj;
    auto p = std::shared_ptr<Packet>(new Packet(*this, pj));
  
  /*
   * Here the packet capture time offset must be added to the system boot time stored in epoch format.
   * See line 58 of this file to see system boot time stored in epoch algorithm
   */

    struct timeval tp;
    std::chrono::system_clock::duration tp_dur = (timeP + temp_offset).time_since_epoch();
    to_timeval(tp_dur, tp);

    p->setTimestampSeconds((uint32_t) tp.tv_sec);
    p->setTimestampMicroseconds((uint32_t) tp.tv_usec);
    p->setPacketlen((uint32_t) packet.size());
    if (p->getPacketlen() > filters->getSnaplen()) {
      p->setCapturelen(filters->getSnaplen());
    } else {
      p->setCapturelen(p->getPacketlen());
    }
    p->setRawPacketData(packet);
    packets_captured.push_back(p);
}

void Packetcapture::addPacket(const PacketJsonObject &value) {
  throw std::runtime_error("Packetcapture::addPacket: Method not implemented");
}

void Packetcapture::replacePacket(const PacketJsonObject &conf) {
  PacketcaptureBase::replacePacket(conf);
}

void Packetcapture::delPacket() {
  throw std::runtime_error("Packetcapture::delPacket: method not implemented");
}

void Packetcapture::attach() {
  dt = std::string("");
  logger()->debug("{0} attached", this->get_name());
}

void Packetcapture::updateFiltersMaps() {
  filters_table ft;
  
  if (filters->l4proto_is_set()) {
    ft.l4proto_flag = true;
    filters->getL4proto().compare("tcp") == 0? ft.l4proto_filter = 1 : ft.l4proto_filter = 2;
  } else {
    ft.l4proto_flag = false;
  }

  if (filters->srcPort_is_set()) {
    ft.src_port_flag = true;
    ft.src_port_filter = filters->getSport();
  } else {
    ft.src_port_flag = false;
  }

  if (filters->dstPort_is_set()) {
    ft.dst_port_flag = true;
    ft.dst_port_filter = filters->getDport();
  } else {
    ft.dst_port_flag = false;
  }

  if (filters->srcIp_is_set()) {
    ft.network_filter_src_flag = true;
    ft.network_filter_src = filters->getNetworkFilterSrc();
    ft.netmask_filter_src = filters->getNetmaskFilterSrc();
    logger()->debug("network filter: {0}\nnetmask filter: {1}", ft.network_filter_src, ft.netmask_filter_src);
  } else {
    ft.network_filter_src_flag = false;
  }

  if (filters->dstIp_is_set()) {
    ft.network_filter_dst_flag = true;
    ft.network_filter_dst = filters->getNetworkFilterDst();
    ft.netmask_filter_dst = filters->getNetmaskFilterDst();
  } else {
    ft.network_filter_dst_flag = false;
  }

  ft.snaplen = filters->getSnaplen();

  auto ft_fast_in = get_array_table<filters_table>("filters_map", 0, ProgramType::INGRESS);
  auto ft_fast_out = get_array_table<filters_table>("filters_map", 0, ProgramType::EGRESS);

  ft_fast_in.set(0x0, ft);
  ft_fast_out.set(0x0, ft);
}

std::shared_ptr<Globalheader> Packetcapture::getGlobalheader() {
  return global_header;
}

void Packetcapture::addGlobalheader(const GlobalheaderJsonObject &value) {
    global_header = std::shared_ptr<Globalheader>(new Globalheader(*this, value));
    global_header->setSnaplen(filters->getSnaplen());
}

void Packetcapture::replaceGlobalheader(const GlobalheaderJsonObject &conf) {
  PacketcaptureBase::replaceGlobalheader(conf);
}

void Packetcapture::delGlobalheader() {
  throw std::runtime_error("Packetcapture::delGlobalheader: method not implemented");
}

void Packetcapture::to_timeval(std::chrono::system_clock::duration& d, struct timeval& tv) {
    std::chrono::seconds const sec = std::chrono::duration_cast<std::chrono::seconds>(d);

    tv.tv_sec  = sec.count();
    tv.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(d - sec).count();
}
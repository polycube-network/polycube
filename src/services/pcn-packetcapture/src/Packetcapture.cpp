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
#include <iostream>
#include <ctime>
#include <chrono>
#include <pcap/pcap.h>
#include "unistd.h"
#include "cbpf2c.h"
#include "Packetcapture.h"
#include "Packetcapture_dp_ingress.h"
#include "Packetcapture_dp_egress.h"
#define BILLION 1000000000

typedef u_int bpf_u_int32;

struct pcap_pt{
    bpf_u_int32 ts_sec;
    bpf_u_int32 ts_usec;
    bpf_u_int32 len;
    bpf_u_int32 caplen;
};

Packetcapture::Packetcapture(const std::string name, const PacketcaptureJsonObject &conf)
  : TransparentCube(conf.getBase(), { packetcapture_code_ingress }, { packetcapture_code_egress }),
    PacketcaptureBase(name) {

  /*
   * The timestamp of the packet is acquired in the fast path using:
   * if BPF_PROG_TYPE_XDP is TC and ctx->tstamp != 0 we use ctx->tstamp
   * it returns the current Unix epoch time
   *
   * otherwise if BPF_PROG_TYPE_XDP is not TC or ctx->tstamp == 0 we use 'bpf_ktime_get_ns()'
   * This function returns the time elapsed since system booted, in nanoseconds.
   *
   * The packet's timestamp must be in epoch so now we will store the current
   * time in epoch format and then the packet capture time offset,that was stored in the fast path,
   * will be added to it.
   *
   * Now we are calculating the system start time in epoch.
   * 'timeval struct ts' rapresents the system start time in epoch calculated as
   * actual time - system uptime
   *
   * See line 167 of this file for more details
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
    setAnonimize(conf.getAnonimize());

  if (conf.dumpIsSet()) {
    setDump(conf.getDump());
  }
  addGlobalheader(conf.getGlobalheader());

  if (conf.snaplenIsSet()) {
    setSnaplen(conf.getSnaplen());
  }

  setNetworkmode(conf.getNetworkmode());

  CapStatus = (uint8_t) conf.getCapture();
  updateFilter();
  cubeType = BaseCube::get_type();
  bootstrap = false;
  writeHeader = true;
  dt = "";
}

std::string Packetcapture::replaceAll(std::string str, const std::string &from, const std::string &to) {
    if (from.empty())
        return nullptr;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length();  // In case 'to' contains 'from', like replacing 'x' with 'yx// '
    }
    return str;
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

    if (dumpFlag == false) {
      //getting temp folder
      //std::filesystem::temp_directory_path only in c++17
      //according to ISO/IEC 9945 (POSIX)
      char const *folder = getenv("TMPDIR");
      if (folder == 0) {
        folder = getenv("TMP");
        if (folder == 0) {
          folder = getenv("TEMP");
          if (folder == 0) {
            folder = getenv("TEMPDIR");
            if (folder == 0) {
              folder = "/tmp";
            }
          }
        }
      }
      temp_folder = std::string(folder);
      temp_folder.append("/");
      random_number = std::to_string(rand() % 1000);    //to avoid two files that using the same name
    }
  }

  if (dumpFlag == true) {
    if (writeHeader == true) {
      myFile.open(temp_folder + ".pcap", std::ios::binary | std::ios::trunc);
    } else {
      myFile.open(temp_folder + ".pcap", std::ios::binary | std::ios::app);
    }
  } else {
    myFile.open(temp_folder + "capture_" + dt + "_" + random_number + ".pcap", std::ios::binary | std::ios::app);
  }

  if (writeHeader == true) {
    struct pcap_file_header *pcap_header = new struct pcap_file_header;
    pcap_header->magic = global_header->getMagic();
    pcap_header->version_major = global_header->getVersionMajor();
    pcap_header->version_minor = global_header->getVersionMinor();
    pcap_header->thiszone = global_header->getThiszone();   //timestamp are always in GMT
    pcap_header->sigfigs = global_header->getSigfigs();
    pcap_header->snaplen = getSnaplen();
    pcap_header->linktype = global_header->getLinktype();

    myFile.write(reinterpret_cast<const char*>(pcap_header), sizeof(*pcap_header));
    writeHeader = false;
  }

  struct timeval tp;
  if (ts) {
      /*
       * Here the packet capture time offset must be added to the system boot time stored in epoch format.
       * See line 64 of this file to see system boot time stored in epoch algorithm
       */
      std::chrono::system_clock::duration tp_dur = (timeP + temp_offset).time_since_epoch();
      to_timeval(tp_dur, tp);
  } else {
      to_timeval(temp_offset, tp);
  }

  p->setTimestampSeconds((uint32_t) tp.tv_sec);
  p->setTimestampMicroseconds((uint32_t) tp.tv_usec);
  p->setPacketlen((uint32_t) packet.size());
  if (p->getPacketlen() > getSnaplen()) {
    p->setCapturelen(getSnaplen());
  } else {
    p->setCapturelen(p->getPacketlen());
  }
  p->setRawPacketData(packet);

  struct pcap_pt *pkt_hdr = new struct pcap_pt;
  pkt_hdr->ts_sec = p->getTimestampSeconds();
  pkt_hdr->ts_usec = p->getTimestampMicroseconds();
  pkt_hdr->len = p->getPacketlen();
  pkt_hdr->caplen = p->getCapturelen();
  getSnaplen() < p->getRawPacketData().size() ? len = getSnaplen() : len = p->getRawPacketData().size();

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
  uint64_t t;
  ts = false;

  switch (direction) {
    case polycube::service::Direction::INGRESS:
    t = (((uint64_t) (md.metadata[1]) << 32) | (uint64_t) md.metadata[0]);
    if (md.metadata[2] == 0) {
        ts = true;
    }
    temp_offset = std::chrono::nanoseconds(t);
    if (getNetworkmode() == true) {
      addPacket(packet);    /* store the packet in the FIFO queue*/
    } else {
      writeDump(packet);
    }
    break;
    case polycube::service::Direction::EGRESS:
    t = (((uint64_t) (md.metadata[1]) << 32) | (uint64_t) md.metadata[0]);
    if (md.metadata[2] == 0) {
        ts = true;
    }
    temp_offset = std::chrono::nanoseconds(t);
    if (getNetworkmode() == true) {
      addPacket(packet);    /* store the packet in the FIFO queue*/
    } else {
      writeDump(packet);
    }
    break;
  }
}

PacketcaptureCaptureEnum Packetcapture::getCapture() {
  return static_cast<PacketcaptureCaptureEnum>(CapStatus);
}

void Packetcapture::setCapture(const PacketcaptureCaptureEnum &value) {
  switch (value) {
    case PacketcaptureCaptureEnum::INGRESS:
      CapStatus = 0;
    break;
    case PacketcaptureCaptureEnum::EGRESS:
      CapStatus = 1;
    break;
    case PacketcaptureCaptureEnum::BIDIRECTIONAL:
      CapStatus = 2;
    break;
    case PacketcaptureCaptureEnum::OFF:
      CapStatus = 3;
    break;
  }
}

bool Packetcapture::getAnonimize() {
  //TODO
}

void Packetcapture::setAnonimize(const bool &value) {
  //TODO
}

std::string Packetcapture::getDump() {
  std::stringstream dump;

  if (network_mode_flag) {
    dump << "the service is running in network mode";
  } else if (temp_folder != "" || dumpFlag == true) {
      char *cwdr_ptr = get_current_dir_name();
      std::string wdr(cwdr_ptr);
      if (dumpFlag == true) {
        dump << "capture dump in " << temp_folder + ".pcap" << std::endl;
      } else {
        dump << "capture dump in " << temp_folder << "capture_" + dt + "_" + random_number + ".pcap" << std::endl;
      }
  } else {
    dump << "no packets captured";
  }

  return dump.str();
}

void Packetcapture::setDump(const std::string &value) {
    dumpFlag = true;
    temp_folder = value;
    writeHeader = true;
}

bool Packetcapture::getNetworkmode() {
  return network_mode_flag;
}

void Packetcapture::setNetworkmode(const bool &value) {
  network_mode_flag = value;
}

/* return the first packet in the queue if exist */
std::shared_ptr<Packet> Packetcapture::getPacket() {
  if (packets_captured.size() != 0) {
    auto p = packets_captured.front();
    if (network_mode_flag) {                            /* pop this element */
      packets_captured.erase(packets_captured.begin());
      if (getSnaplen() < p->getPacketlen()) {
        p->packet.resize(getSnaplen());
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

    struct timeval tp;
    if (ts) {
        /*
         * Here the packet capture time offset must be added to the system boot time stored in epoch format.
         * See line 64 of this file to see system boot time stored in epoch algorithm
         */
        std::chrono::system_clock::duration tp_dur = (timeP + temp_offset).time_since_epoch();
        to_timeval(tp_dur, tp);
    } else {
        to_timeval(temp_offset, tp);
    }
    p->setTimestampSeconds((uint32_t) tp.tv_sec);
    p->setTimestampMicroseconds((uint32_t) tp.tv_usec);
    p->setPacketlen((uint32_t) packet.size());
    if (p->getPacketlen() > getSnaplen()) {
      p->setCapturelen(getSnaplen());
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

void Packetcapture::updateFilter() {
    if (bootstrap)
        filterCode = "return RX_OK;"; // Default filter captures no packets (the eBPF datapath simply returns ok)

    std::string codeINGRESS = replaceAll(packetcapture_code_ingress, "//CUSTOM_FILTER_CODE", filterCode);
    std::string codeEGRESS = replaceAll(packetcapture_code_egress, "//CUSTOM_FILTER_CODE", filterCode);

    if (CapStatus == 3 || CapStatus == 2) {
      reload(codeINGRESS,0,ProgramType::INGRESS);
      reload(codeEGRESS,0,ProgramType::EGRESS);
    } else {
        if (CapStatus == 0) {
          reload(codeINGRESS,0,ProgramType::INGRESS);
        } else {
            reload(codeEGRESS,0,ProgramType::EGRESS);
        }
    }

    logger()->debug("FilterCode:\n{0}", filterCode);
}

std::shared_ptr<Globalheader> Packetcapture::getGlobalheader() {
  return global_header;
}

void Packetcapture::addGlobalheader(const GlobalheaderJsonObject &value) {
    global_header = std::shared_ptr<Globalheader>(new Globalheader(*this, value));
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

void Packetcapture::setFilter(const std::string &value) {
    logger()->info("Inserted filter: {0}", value);
    if (value == "all") {
        if (cubeType == polycube::CubeType::TC) {
            filterCode = "if (ctx->tstamp == 0) {\n\tpkt_timestamp = bpf_ktime_get_ns();\n\tmdata[0] = *(&pkt_timestamp);\n\t"
                         "mdata[1] = (*(&pkt_timestamp)) >> 32;\n\tmdata[2] = 0;\n} else {\n\tmdata[0] = *(&ctx->tstamp);\n\t"
                         "mdata[1] = (*(&ctx->tstamp)) >> 32;\n\tmdata[2] = 1;\n}\npcn_pkt_controller_with_metadata(ctx, md, reason, mdata);\n"
                         "return RX_OK;";
        } else { /* CubeType::XDP_DRV or CubeType::XDP_SKB */
            filterCode = "pkt_timestamp = bpf_ktime_get_ns();\nmdata[0] = *(&pkt_timestamp);\n"
                         "mdata[1] = (*(&pkt_timestamp)) >> 32;\nmdata[2] = 0;\npcn_pkt_controller_with_metadata(ctx, md, reason, mdata);\n"
                         "return RX_OK;";
        }
    } else {
        memset(&cbpf, 0, sizeof(cbpf));
        filterCompile(value, &cbpf);
    }
    this->filter = value;
    updateFilter();
}

std::string Packetcapture::getFilter() {
  return this->filter;
}

uint32_t Packetcapture::getSnaplen() {
  return global_header->getSnaplen();
}

void Packetcapture::setSnaplen(const uint32_t &value) {
    global_header->setSnaplen(value);
}

void Packetcapture::filterCompile(std::string str, struct sock_fprog * cbpf) {
    int ret,i;
    struct bpf_program _bpf;
    const struct bpf_insn *ins;
    struct sock_filter *out;
    int link_type = 1;   //ETHERNET type

    ret = pcap_compile_nopcap(65535, link_type, &_bpf, str.c_str(), 1, 0xffffffff);
    if (ret < 0) {
      logger()->error("Cannot compile filter: {0}", str);
      throw std::runtime_error("Filter not valid");
    }

    cbpf->len = _bpf.bf_len;
    /*
     * Here I realloc the struct that will contains the cBPF instructions to the right
     * size: _bpf len * sizeof(sock_filter struct) and then I do the static_cast because the
     * realloc returns a void*
     */
    cbpf->filter = static_cast<sock_filter *>(realloc(cbpf->filter, cbpf->len * sizeof(*out)));

    for (i = 0, ins = _bpf.bf_insns, out = cbpf->filter; i < cbpf->len; ++i, ++ins, ++out) {
        out->code = ins->code;
        out->jt = ins->jt;
        out->jf = ins->jf;
        out->k = ins->k;

        if (out->code == 0x06 && out->k > 0)
            out->k = 0xFFFFFFFF;
    }

    pcap_freecode(&_bpf);
    filterCode = cbpf2c::ToC(cbpf, cubeType);
}
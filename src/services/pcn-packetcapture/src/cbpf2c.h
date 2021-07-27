#ifndef POLYCUBE_CBPF2C_H
#define POLYCUBE_CBPF2C_H

#include <string>
#include <linux/filter.h>
#include <polycube/services/cube_iface.h>


class cbpf2c {

public:

  static std::string ToC(struct sock_fprog * cbpf, polycube::service::CubeType cubeType);

  static int cbpf_validate(const struct sock_fprog *bpf);

  static std::string bpf_dump_all(struct sock_fprog *bpf, polycube::service::CubeType cubeType);

  static char *_cbpf_dump(const struct sock_filter bpf, int n, polycube::service::CubeType cubeType);

  static const char *cbpf_dump_linux_k(uint32_t k);
};


#endif //POLYCUBE_CBPF2C_H
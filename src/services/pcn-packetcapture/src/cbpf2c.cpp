/*
 * For refernce see https://github.com/cloudflare/cbpfc (cloudflare's project that can compile cBPF to eBPF, or to C)
 */

#include <iostream>
#include "cbpf2c.h"
#include "linux/bpf.h"
#include <arpa/inet.h>
#include <linux/filter.h>
#include <map>
#include <cstring>

#ifndef BPF_MEMWORDS
# define BPF_MEMWORDS	16
#endif

#define BPF_LD_B	(BPF_LD   |    BPF_B)
#define BPF_LD_H	(BPF_LD   |    BPF_H)
#define BPF_LD_W	(BPF_LD   |    BPF_W)
#define BPF_LDX_B	(BPF_LDX  |    BPF_B)
#define BPF_JMP_JNE	(BPF_JMP  |  BPF_JNE)
#define BPF_JMP_JEQ	(BPF_JMP  |  BPF_JEQ)
#define BPF_JMP_JGT	(BPF_JMP  |  BPF_JGT)
#define BPF_JMP_JGE	(BPF_JMP  |  BPF_JGE)
#define BPF_JMP_JLT	(BPF_JMP  |  BPF_JLT)
#define BPF_JMP_JLE	(BPF_JMP  |  BPF_JLE)
#define BPF_JMP_JSET	(BPF_JMP  | BPF_JSET)
#define BPF_ALU_ADD	(BPF_ALU  |  BPF_ADD)
#define BPF_ALU_SUB	(BPF_ALU  |  BPF_SUB)
#define BPF_ALU_MUL	(BPF_ALU  |  BPF_MUL)
#define BPF_ALU_DIV	(BPF_ALU  |  BPF_DIV)
#define BPF_ALU_MOD	(BPF_ALU  |  BPF_MOD)
#define BPF_ALU_NEG	(BPF_ALU  |  BPF_NEG)
#define BPF_ALU_AND	(BPF_ALU  |  BPF_AND)
#define BPF_ALU_OR	(BPF_ALU  |   BPF_OR)
#define BPF_ALU_XOR	(BPF_ALU  |  BPF_XOR)
#define BPF_ALU_LSH	(BPF_ALU  |  BPF_LSH)
#define BPF_ALU_RSH	(BPF_ALU  |  BPF_RSH)
#define BPF_MISC_TAX	(BPF_MISC |  BPF_TAX)
#define BPF_MISC_TXA	(BPF_MISC |  BPF_TXA)


std::string cbpf2c::ToC(struct sock_fprog * cbpf, polycube::service::CubeType cubeType) {
    /* First I validate the cBPF code */
    if (cbpf_validate(cbpf) == 0)
        std::cout<<"Not a valid cBPF program!"<<std::endl;
    std::string ret = bpf_dump_all(cbpf, cubeType);
    return ret;
}


std::string cbpf2c::bpf_dump_all(struct sock_fprog *bpf, polycube::service::CubeType cubeType) {
    int i;
    std::string ret = "";
    for (i = 0; i < bpf->len; ++i) {
        /* Then I translate the cBPF code into C code */
        char *str = _cbpf_dump(bpf->filter[i], i, cubeType);
        ret += std::string(str);
    }
    return ret;
}


int cbpf2c::cbpf_validate(const struct sock_fprog *bpf) {
    uint32_t i, from;
    const struct sock_filter *p;

    if (!bpf)
        return 0;
    if (bpf->len < 1)
        return 0;

    for (i = 0; i < bpf->len; ++i) {
        p = &bpf->filter[i];
        switch (BPF_CLASS(p->code)) {
        /* Check that memory operations use valid addresses */
        case BPF_LD:
        case BPF_LDX:
            switch (BPF_MODE(p->code)) {
            case BPF_IMM:
                break;
            case BPF_ABS:
            case BPF_IND:
            case BPF_MSH:
                break;
            case BPF_MEM:
                if (p->k >= BPF_MEMWORDS)
                    return 0;
                break;
            case BPF_LEN:
                break;
            default:
                return 0;
            }
            break;
        case BPF_ST:
        case BPF_STX:
            if (p->k >= BPF_MEMWORDS)
                return 0;
            break;
        case BPF_ALU:
            switch (BPF_OP(p->code)) {
            case BPF_ADD:
            case BPF_SUB:
            case BPF_MUL:
            case BPF_OR:
            case BPF_XOR:
            case BPF_AND:
            case BPF_LSH:
            case BPF_RSH:
            case BPF_NEG:
                break;
            case BPF_DIV:
            case BPF_MOD:
                //Check for constant division by 0 (undefine for div and mod).
                if (BPF_RVAL(p->code) == BPF_K && p->k == 0)
                    return 0;
                break;
            default:
                return 0;
            }
            break;
        case BPF_JMP:
            /* Check that jumps are within the code block,
             * and that unconditional branches don't go
             * backwards as a result of an overflow.
             * Unconditional branches have a 32-bit offset,
             * so they could overflow; we check to make
             * sure they don't.  Conditional branches have
             * an 8-bit offset instead.
             */
            from = i + 1;
            switch (BPF_OP(p->code)) {
            case BPF_JA:
                if (from + p->k >= bpf->len)
                    return 0;
                break;
            case BPF_JEQ:
            case BPF_JGT:
            case BPF_JGE:
            case BPF_JSET:
                if (from + p->jt >= bpf->len ||
                    from + p->jf >= bpf->len)
                    return 0;
                break;
            default:
                return 0;
            }
            break;
        case BPF_RET:
            break;
        case BPF_MISC:
            break;
        }
    }

    return BPF_CLASS(bpf->filter[bpf->len - 1].code) == BPF_RET;
}


char *cbpf2c::_cbpf_dump(const struct sock_filter bpf, int n, polycube::service::CubeType cubeType)
{
    int v;
    const char *fmt, *op;
    static char ret[512];
    char operand[64];

    v = bpf.k;
    /*
     * Switch over the type of instruction of the code and prepare two variables
     * op (operation) and fmt (operand)
     */
    switch (bpf.code) {
    case BPF_RET | BPF_K:
        op = "return";
        fmt = "0x%x";
        break;
    case BPF_RET | BPF_A:
        op = "return";
        fmt = "a";
        break;
    case BPF_RET | BPF_X:
        op = "return";
        fmt = "x";
        break;
    case BPF_LD_W | BPF_ABS:
        op = "ld";
        fmt = cbpf_dump_linux_k(bpf.k);
        break;
    case BPF_LD_H | BPF_ABS:
        op = "ldh";
        fmt = cbpf_dump_linux_k(bpf.k);
        break;
    case BPF_LD_B | BPF_ABS:
        op = "ldb";
        fmt = cbpf_dump_linux_k(bpf.k);
        break;
    case BPF_LD_W | BPF_LEN:
        op = "ld";
        fmt = "len";
        break;
    case BPF_LD_W | BPF_IND:
        op = "ld_ind";
        fmt = "x + %d";
        break;
    case BPF_LD_H | BPF_IND:
        op = "ldh_ind";
        fmt = "x + %d";
        break;
    case BPF_LD_B | BPF_IND:
        op = "ldb_ind";
        fmt = "x + %d";
        break;
    case BPF_LD | BPF_IMM:
        op = "ld_imm";
        fmt = "0x%x";
        break;
    case BPF_LDX | BPF_IMM:
        op = "ldx_imm";
        fmt = "0x%x";
        break;
    case BPF_LDX_B | BPF_MSH:
        op = "ldxb";
        fmt = "4*([%d]&0xf)";
        break;
    case BPF_LD | BPF_MEM:
        op = "ld_mem";
        fmt = "m[%d]";
        break;
    case BPF_LDX | BPF_MEM:
        op = "ldx_mem";
        fmt = "m[%d]";
        break;
    case BPF_ST:
        op = "st";
        fmt = "m[%d]";
        break;
    case BPF_STX:
        op = "stx";
        fmt = "m[%d]";
        break;
    case BPF_JMP_JGT | BPF_K:
        op = "if(a >";
        fmt = "0x0%x";
        break;
    case BPF_JMP_JGE | BPF_K:
        op = "if(a >=";
        fmt = "0x0%x";
        break;
    case BPF_JMP_JLT | BPF_K:
        op = "if(a <";
        fmt = "0x0%x";
        break;
    case BPF_JMP_JLE | BPF_K:
        op = "if(a <=";
        fmt = "0x0%x";
        break;
    case BPF_JMP_JNE | BPF_K:
        op = "if(a !=";
        fmt = "0x0%x";
        break;
    case BPF_JMP_JEQ | BPF_K:
        op = "if(a ==";
        fmt = "0x0%x";
        break;
    case BPF_JMP_JSET | BPF_K:
        op = "if(a &";
        fmt = "0x0%x";
        break;
    case BPF_JMP_JGT | BPF_X:
        op = "if(a >";
        fmt = "x";
        break;
    case BPF_JMP_JGE | BPF_X:
        op = "if(a >=";
        fmt = "x";
        break;
    case BPF_JMP_JLT | BPF_X:
        op = "if(a <";
        fmt = "x";
        break;
    case BPF_JMP_JLE | BPF_X:
        op = "if(a <=";
        fmt = "x";
        break;
    case BPF_JMP_JNE | BPF_X:
        op = "if(a !=";
        fmt = "x";
        break;
    case BPF_JMP_JEQ | BPF_X:
        op = "if(a ==";
        fmt = "x";
        break;
    case BPF_JMP_JSET | BPF_X:
        op = "if(a &";
        fmt = "x";
        break;
    case BPF_ALU_ADD | BPF_X:
        op = "+";
        fmt = "a += x";
        break;
    case BPF_ALU_SUB | BPF_X:
        op = "-";
        fmt = "a -= x";
        break;
    case BPF_ALU_MUL | BPF_X:
        op = "*";
        fmt = "a *= x";
        break;
    case BPF_ALU_DIV | BPF_X:
        op = "/";
        fmt = "a /= x";
        break;
    case BPF_ALU_MOD | BPF_X:
        op = "%";
        fmt = "a % x";
        break;
    case BPF_ALU_AND | BPF_X:
        op = "&";
        fmt = "a & x";
        break;
    case BPF_ALU_OR | BPF_X:
        op = "|";
        fmt = "a | x";
        break;
    case BPF_ALU_XOR | BPF_X:
        op = "^";
        fmt = "a ^ x";
        break;
    case BPF_ALU_LSH | BPF_X:
        op = "<<";
        fmt = "a << x";
        break;
    case BPF_ALU_RSH | BPF_X:
        op = ">>";
        fmt = "a >> x";
        break;
    case BPF_ALU_ADD | BPF_K:
        op = "+";
        fmt = "a +=%d";
        break;
    case BPF_ALU_SUB | BPF_K:
        op = "-";
        fmt = "a -=%d";
        break;
    case BPF_ALU_MUL | BPF_K:
        op = "*";
        fmt = "a *=%d";
        break;
    case BPF_ALU_DIV | BPF_K:
        op = "/";
        fmt = "a /=%d";
        break;
    case BPF_ALU_MOD | BPF_K:
        op = "%";
        fmt = "a % %d";
        break;
    case BPF_ALU_AND | BPF_K:
        op = "&";
        fmt = "a & 0x%x";
        break;
    case BPF_ALU_OR | BPF_K:
        op = "|";
        fmt = "a | 0x%x";
        break;
    case BPF_ALU_XOR | BPF_K:
        op = "^";
        fmt = "a ^ 0x%x";
        break;
    case BPF_ALU_LSH | BPF_K:
        op = "<<";
        fmt = "a << %d";
        break;
    case BPF_ALU_RSH | BPF_K:
        op = ">>";
        fmt = "a >> %d";
        break;
    case BPF_ALU_NEG:
        op = "a = -a;";
        fmt = "";
        break;
    case BPF_MISC_TAX:
        op = "a = x;";
        fmt = "";
        break;
    case BPF_MISC_TXA:
        op = "x = a;";
        fmt = "";
        break;
    default:
        op = "unimp";
        fmt = "0x%x";
        v = bpf.code;
        break;
    }
    snprintf(operand, sizeof(operand), fmt, v);
    operand[sizeof(operand) - 1] = '\0';

    /*
     * Here I generate the C code instruction according to the type of instruction (e.g.,return, load, store, alu op. etc.)
     */
    if (strcmp(op,"unimp")==0) {
      snprintf(ret, sizeof(ret), "L%d:\t unimp", n);
    }else {
        if ((BPF_CLASS(bpf.code) == BPF_JMP && BPF_OP(bpf.code) != BPF_JA)) {
          snprintf(ret, sizeof(ret), "L%d:\t%s %s) {\n\t\tgoto L%d;\n\t}else{\n\t\tgoto L%d;\n\t}\n", n, op, operand, n + 1 + bpf.jt, n + 1 + bpf.jf);
        } else {
            if ((BPF_CLASS(bpf.code) == BPF_RET)) {
                if (strcmp(operand, "0x0") == 0) {
                  snprintf(ret, sizeof(ret), "L%d:\t%s RX_OK;", n, op);
                } else if (strcmp(operand, "0xffffffff") == 0) {
                  if (cubeType == polycube::CubeType::TC) {
                    snprintf(ret, sizeof(ret), "L%d:\tif (ctx->tstamp == 0) {\n\t\tpkt_timestamp = bpf_ktime_get_ns();\n\t\tmdata[0] = *(&pkt_timestamp);\n\t\t"
                                                "mdata[1] = (*(&pkt_timestamp)) >> 32;\n\t\tmdata[2] = 0;\n\t} else {\n\t\tmdata[0] = *(&ctx->tstamp);\n\t\t"
                                                "mdata[1] = (*(&ctx->tstamp)) >> 32;\n\t\tmdata[2] = 1;\n\t}\n\t%s pcn_pkt_controller_with_metadata(ctx, md, reason, mdata);\n", n, op);
                  } else { /* CubeType::XDP_DRV or CubeType::XDP_SKB */
                    snprintf(ret, sizeof(ret), "L%d:\tpkt_timestamp = bpf_ktime_get_ns();\n\tmdata[0] = *(&pkt_timestamp);\n\t"
                    "mdata[1] = (*(&pkt_timestamp)) >> 32;\n\tmdata[2] = 0;\n\t%s pcn_pkt_controller_with_metadata(ctx, md, reason, mdata);\n", n, op);
                  }
                } else {
                  snprintf(ret, sizeof(ret), "L%d:\t unimp\n", n);
                }
            } else {
                if ((BPF_CLASS(bpf.code) == BPF_MISC_TAX) || (BPF_CLASS(bpf.code) == BPF_MISC_TXA)) {
                  snprintf(ret, sizeof(ret), "L%d:\t%s\n", n, op);
                } else {
                    if (BPF_CLASS(bpf.code) == BPF_ST) {
                      snprintf(ret, sizeof(ret), "L%d:\t//(%s)\n%s=a;\n", n, op, operand);
                    } else {
                        if ((BPF_CLASS(bpf.code) == BPF_STX)) {
                          snprintf(ret, sizeof(ret), "L%d:\t//(%s)\n%s=x;\n", n, op, operand);
                        } else {
                            if ((bpf.code) == (BPF_LD_B | BPF_ABS)) {
                              char operand2[64];
                              snprintf(operand2, sizeof(operand), fmt, v + 1);
                              operand2[sizeof(operand2) - 1] = '\0';
                              snprintf(ret, sizeof(ret),"L%d:\t//(%s)\n\tif((data + %s) > data_end){\n\t\treturn RX_DROP;\n\t}\n\ta = * ((uint8_t *) &data[%s]);\n",n, op, operand2, operand);
                            } else {
                                if ((bpf.code) == (BPF_LDX_B | BPF_MSH)){
                                  char operand2[64];
                                  snprintf(operand2, sizeof(operand), fmt, v + 1);
                                  operand2[sizeof(operand2) - 1] = '\0';
                                  snprintf(ret, sizeof(ret),"L%d:\t//(%s)\n\tif((data + %s) > data_end){\n\t\treturn RX_DROP;\n\t}\n\tx = * ((uint8_t *) &data[%s]);\n",n, op, operand2, operand);
                                } else {
                                    if ((bpf.code) == (BPF_LD_H | BPF_ABS)) {
                                      char operand2[64];
                                      snprintf(operand2, sizeof(operand), fmt, v + 2);
                                      operand2[sizeof(operand2) - 1] = '\0';
                                      snprintf(ret, sizeof(ret),"L%d:\t//(%s)\n\tif((data + %s) > data_end){\n\t\treturn RX_DROP;\n\t}\n\ta = ntohs(* ((uint16_t *) &data[%s]));\n",n, op, operand2, operand);
                                    } else {
                                        if (BPF_CLASS(bpf.code) == BPF_LD) {
                                          char operand2[64];
                                          snprintf(operand2, sizeof(operand), fmt, v + 4);
                                          operand2[sizeof(operand2) - 1] = '\0';
                                          snprintf(ret, sizeof(ret),"L%d:\t//(%s)\n\tif((data + %s) > data_end){\n\t\treturn RX_DROP;\n\t}\n\ta = ntohl(* ((uint32_t *) &data[%s]));\n",n, op, operand2, operand);
                                        } else {
                                            if (BPF_CLASS(bpf.code) == BPF_LDX) {
                                              char operand2[64];
                                              snprintf(operand2, sizeof(operand), fmt, v + 4);
                                              operand2[sizeof(operand2) - 1] = '\0';
                                              snprintf(ret, sizeof(ret),"L%d:\t//(%s)\n\tif((data + %s) > data_end){\n\t\treturn RX_DROP;\n\t}\n\tx = ntohl(* ((uint32_t *) &data[%s]));\n",n, op, operand2, operand);
                                            } else {
                                              snprintf(ret, sizeof(ret), "L%d:\t%s;\n", n, operand);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    ret[sizeof(ret) - 1] = '\0';
    return ret;
}

/*
 * Function called when the type of instruction is a LOAD with a fixed offset (BPF_ABS)
 * to switch over the generic multiuse field K
 */
const char *cbpf2c::cbpf_dump_linux_k(uint32_t k) {
    switch (k) {
    default:
        return "%d";
    case static_cast<uint32_t>(SKF_AD_OFF + SKF_AD_PROTOCOL):
        return "proto";
    case static_cast<uint32_t>(SKF_AD_OFF + SKF_AD_PKTTYPE):
        return "type";
    case static_cast<uint32_t>(SKF_AD_OFF + SKF_AD_IFINDEX):
        return "ifidx";
    case static_cast<uint32_t>(SKF_AD_OFF + SKF_AD_NLATTR):
        return "nla";
    case static_cast<uint32_t>(SKF_AD_OFF + SKF_AD_NLATTR_NEST):
        return "nlan";
    case static_cast<uint32_t>(SKF_AD_OFF + SKF_AD_MARK):
        return "mark";
    case static_cast<uint32_t>(SKF_AD_OFF + SKF_AD_QUEUE):
        return "queue";
    case static_cast<uint32_t>(SKF_AD_OFF + SKF_AD_HATYPE):
        return "hatype";
    case static_cast<uint32_t>(SKF_AD_OFF + SKF_AD_RXHASH):
        return "rxhash";
    case static_cast<uint32_t>(SKF_AD_OFF + SKF_AD_CPU):
        return "cpu";
    case static_cast<uint32_t>(SKF_AD_OFF + SKF_AD_VLAN_TAG):
        return "vlant";
    case static_cast<uint32_t>(SKF_AD_OFF + SKF_AD_VLAN_TAG_PRESENT):
        return "vlanp";
    case static_cast<uint32_t>(SKF_AD_OFF + SKF_AD_PAY_OFFSET):
        return "poff";
    }
}
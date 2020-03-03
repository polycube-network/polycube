Packetcapture service filter
============================

Packetcapture filter is a tcpdump like filter that allows to insert a specific rule using tcpdump syntax.


How it works
------------
The pipeline to convert the filter entered in the packetcapture service to C code is:

**pcap filter** → *libpcap* → **cBPF** → *cbpf2c* → **C code**


More in details, the first step is to obtain the cBPF code from the string value of the filter inserted,
it can be done using the pcap_compile_nopcap function that returns a bpf_program structure containing the bpf_insn.

Then will be created a sock_fprog structure called cbpf that will contains all the filter blocks needed.

The second step (traslation from cBPF to C) starts with the validation of the cBPF code, and then for each filter block
is called _cbpf_dump function that will return a string containing the equivalent C code for that block.

Inside the _cbpf_dump function there is a switch statement that will prepare two variables, op (operation) and fmt (operand),
depending on the type of instruction of the block (e.g.,return, load, store, alu op. etc.), then they will be used to
generate the C code

For reference see: `cloudflare project <https://blog.cloudflare.com/xdpcap/>`__


Example of C code generated
---------------------------
Here is the generated C code for the filter "icmp":

::

    L0:	if ((data + 14) > data_end) {
            return RX_DROP;
        }
        a = ntohs(* ((uint16_t *) &data[12]));
    L1:	if (a == 0x0800) {
            goto L2;
        } else {
            goto L5;
        }
    L2:	if ((data + 24) > data_end) {
            return RX_DROP;
        }
        a = * ((uint8_t *) &data[23]);
    L3:	if (a == 0x01) {
            goto L4;
        } else {
            goto L5;
        }
    L4:	return pcn_pkt_controller(ctx, md, reason);
    L5:	return RX_OK;
#!/bin/bash
# run_target.sh
# Helper script to run Qemu such that the kernel waits in early boot for the
# GDB client to connect
# (Part of content covered in Linux Kernel Debugging, Kaiwan N Billimoria, Packt
#  Ch 11 - Using Kernel GDB (KGDB))
name=$(basename $0)
[ $# -ne 2 ] && {
  echo "Usage: ${name} path-to-kernel-[b]zimage path-to-rootfs-image"
  exit 1
}
KERNIMG=$1
ROOTFS=$2

[ ! -f ${KERNIMG} ] && {
  echo "${name}: kernel image \"$1\" not found? aborting"; exit 1
}
[ ! -f ${ROOTFS} ] && {
  echo "${name}: rootfs image \"$2\" not found? aborting"; exit 1
}
RAM=1G
CPU_CORES=2

echo "Note:
1. First shut down any other hypervisor instance that may be running
2. Once run, this guest qemu system will *wait* for GDB to connect from the host:
On the host, do:

$ gdb -q <linux-src-tree>/vmlinux
(gdb) target remote :1234
"
echo "qemu-system-x86_64 \
 -kernel ${KERNIMG}
 -append "console=ttyS0 root=/dev/sda earlyprintk=serial rootfstype=ext4 rootwait nokaslr" \
 -hda ${ROOTFS} \
 -nographic -m ${RAM} -smp ${CPU_CORES} \
 -S -s"
qemu-system-x86_64 \
 -kernel ${KERNIMG} \
 -append "console=ttyS0 root=/dev/sda earlyprintk=serial rootfstype=ext4 rootwait nokaslr" \
 -hda ${ROOTFS} \
 -nographic -m ${RAM} -smp ${CPU_CORES} \
 -S -s
# -S  Do not start CPU at startup (you must type 'c' in the monitor).
# -s  Shorthand for -gdb tcp::1234, i.e. open a gdbserver on TCP port 1234.

#!/bin/bash
# run.sh
# Helper script to run Qemu such that the kernel waits in early boot for the
# GDB client to connect
# (Part of content covered in Linux Kernel Debugging, Kaiwan N Billimoria, Packt
#  Ch 11 - Using Kernel GDB (KGDB))

### UPDATE this variable as per your setup ###
IMG=~/Linux-Kernel-Debugging/ch11/images

KERNIMG=${IMG}/linux-5.10.3/arch/x86/boot/bzImage
ROOTFS=${IMG}/rootfs.img

echo "Note:
1. First shut down any other hypervisor instance that may be running
2. Once run, this guest qemu system will *wait* for GDB to connect from the host:
On the host, do:

$ gdb -q linux-5.10.3/vmlinux
(gdb) target remote :1234
"
echo "qemu-system-x86_64 \
 -kernel ${KERNIMG}
 -append "console=ttyS0 root=/dev/sda earlyprintk=serial rootfstype=ext4 rootwait nokaslr" \
 -hda ${ROOTFS} \
 -nographic -m 1G -smp 2 \
 -S -s"
qemu-system-x86_64 \
 -kernel ${KERNIMG} \
 -append "console=ttyS0 root=/dev/sda earlyprintk=serial rootfstype=ext4 rootwait nokaslr" \
 -hda ${ROOTFS} \
 -nographic -m 1G -smp 2 \
 -S -s
# -S  Do not start CPU at startup (you must type 'c' in the monitor).
# -s  Shorthand for -gdb tcp::1234, i.e. open a gdbserver on TCP port 1234.

#!/bin/bash
# test.sh
KMOD=3_kprobe
# As pointed out here:
# https://github.com/PacktPublishing/Linux-Kernel-Debugging#updates--observations
# As of now (early 2023), attempting to trace file open's via the do_sys_open()
# doesn't seem to cut it... I find that instead using the do_sys_openat2()
# works! So, substitute this function in place of the do_sys_open() being used
# and you may get better results...
# (In fact, our book's Figure 4.13 shows the do_sys_openat2() being invoked!).
FUNC_TO_KPROBE=do_sys_openat2
#FUNC_TO_KPROBE=do_sys_open
VERBOSE=1

[ $# -ne 1 ] && {
	echo "Usage: $0 USE_VI ; 0 = show for all, 1 = show for vi only"
	exit 1
}
SKIP_NOT_VI=$1
DYNDBG_CTRL=/sys/kernel/debug/dynamic_debug/control
if [ ! -f ${DYNDBG_CTRL} ]; then
   [ -f /proc/dynamic_debug/control ] && DYNDBG_CTRL=/proc/dynamic_debug/control \
	   || DYNDBG_CTRL=""
fi

echo "Module ${KMOD}: function to probe: ${FUNC_TO_KPROBE}
"
if [ ! -f ./${KMOD}.ko ]; then
  echo "Building ${KMOD} ..."
  make || exit 1
fi
sudo rmmod ${KMOD} 2>/dev/null # rm any stale instance
# Ideally, first check that the function to kprobe isn't blacklisted; we skip
# this here, doing this in the more sophisticated ch4/kprobes/4_kprobe_helper/kp_load.sh script
sudo insmod ./${KMOD}.ko kprobe_func=${FUNC_TO_KPROBE} verbose=${VERBOSE} skip_if_not_vi=${SKIP_NOT_VI} || exit 1

[ -z "${DYNDBG_CTRL}" ] && {
   echo "No dynamic debug control file available..."
   exit 1
}

echo "-- Module ${KMOD} now inserted, turn on dynamic debug prints now --"
sudo bash -c "grep \"${KMOD} .* =_ \" ${DYNDBG_CTRL}" && echo "Wrt module ${KMOD}, one or more dynamic debug prints are Off" || \
 echo "Wrt module ${KMOD}, one or more dynamic debug prints are On"
# turn On debug prints
sudo bash -c "echo -n \"module ${KMOD} +p\" > ${DYNDBG_CTRL}"
sudo bash -c "grep \"${KMOD}\" ${DYNDBG_CTRL}"
echo "--   All set, look up kernel log with, f.e., journalctl -k -f   --"
exit 0

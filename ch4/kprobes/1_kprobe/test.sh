#!/bin/bash
# test.sh
KMOD=1_kprobe
VERBOSE=1

DYNDBG_CTRL=/sys/kernel/debug/dynamic_debug/control
if [ ! -f ${DYNDBG_CTRL} ]; then
   [ -f /proc/dynamic_debug/control ] && DYNDBG_CTRL=/proc/dynamic_debug/control \
       || DYNDBG_CTRL=""
fi
[ -z "${DYNDBG_CTRL}" ] && {
   echo "No dynamic debug control file available..."
   exit 1
}

echo "Module ${KMOD}: function to probe: do_sys_open()
"
if [ ! -f ./${KMOD}.ko ]; then
  echo "Building ${KMOD} ..."
  make || exit 1
fi
sudo rmmod ${KMOD} 2>/dev/null # rm any stale instance
sudo insmod ./${KMOD}.ko verbose=${VERBOSE} || exit 1

echo "-- Module ${KMOD} now inserted, turn on any dynamic debug prints now --"
sudo bash -c "grep \"${KMOD} .* =_ \" ${DYNDBG_CTRL}" && echo "Wrt module ${KMOD}, one or more dynamic debug prints are Off" || \
 echo "Wrt module ${KMOD}, one or more dynamic debug prints are On"
# turn On debug prints
sudo bash -c "echo -n \"module ${KMOD} +p\" > ${DYNDBG_CTRL}"
sudo bash -c "grep \"${KMOD}\" ${DYNDBG_CTRL}"
echo "--   All set, look up kernel log with, f.e., journalctl -k -f   --"
exit 0

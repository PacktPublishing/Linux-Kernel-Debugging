#!/bin/bash
# test.sh
KMOD=kprobe_lkm
FUNC_TO_KPROBE=do_sys_open
DYNDBG_CTRL=/proc/dynamic_debug/control  # safe, always there
VERBOSE=0

echo "Module ${KMOD}: function to probe: ${FUNC_TO_KPROBE}
"
if [ ! -f ./${KMOD}.ko ]; then
  echo "Building ${KMOD} ..."
  make || exit 1
fi
sudo rmmod ${KMOD} 2>/dev/null # rm any stale instance
sudo insmod ./${KMOD}.ko kprobe_func=${FUNC_TO_KPROBE} verbose=${VERBOSE} || exit 1

echo "-- Module ${KMOD} now inserted, turn on dynamic debug prints now --"
sudo bash -c "grep \"${KMOD} .* =_ \" ${DYNDBG_CTRL}" && echo "Wrt module ${KMOD}, one or more dynamic debug prints are Off" || \
 echo "Wrt module ${KMOD}, one or more dynamic debug prints are On"
# turn On debug prints
sudo bash -c "echo -n \"module ${KMOD} +p\" > ${DYNDBG_CTRL}"
sudo bash -c "grep \"${KMOD}\" ${DYNDBG_CTRL}"
echo "--   All set, look up kernel log with, f.e., journalctl -k -f   --"


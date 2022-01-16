#!/bin/bash
name=$(basename $0)
KMOD=kcsan_datarace
MAX=10
i=0
SLEEP_TM=3.5  # why? This is as CONFIG_KCSAN_REPORT_ONCE_IN_MS=3000 by default

[ ! -f ${KMOD}.ko ] && {
  echo "${name}: module ${KMOD}.ko not built?"
  exit 1
}
[ $(id -u) -ne 0 ] && {
  echo "${name}: must run as root."
  exit 1
}
[ $# -ne 3 ] && {
  echo "${name} max-tries loops_in_func1 loops_in_func2"
  exit 1
}
MAX=$1
iter1=$2
iter2=$3
rmmod ${KMOD} 2>/dev/null

dmesg -C
while [ $i -lt ${MAX} ]
do
  echo "Trial ${i} -----------------------------------------"
  echo "Trial ${i} -----------------------------------------" > /dev/kmsg
  insmod ./${KMOD}.ko race_2plain_w=y iter1=${iter1} iter2=${iter2}
  rmmod ${KMOD}
  [ ${MAX} -gt 1 ] && sleep ${SLEEP_TM}
  let i=i+1
done

#!/bin/bash
# ch8/tester.sh
# ***************************************************************
# This program is part of the source code released for the book
#  "Linux Kernel Debugging"
#  (c) Author: Kaiwan N Billimoria
#  Publisher:  Packt
#  GitHub repository:
#  https://github.com/PacktPublishing/Linux-Kernel-Debugging
#
# From: Ch 8: Lock Debugging
# ****************************************************************
# A simple wrapper script to test (approx) how many times to minimally loop in
# the 'racy' code within the kernel module, in order to trigger a data race
# that KCSAN can catch!
# TIP:
# If you find that KCSAN's not catching the data race, try increasing the # of
# loops to large values; f.e., invoke it like this:
# ./tester.sh 1 75000 50000
#
# For details, please refer the book, Ch 8.
name=$(basename $0)
KCONF=/boot/config-$(uname -r)
KMOD=kcsan_datarace
MAX=10
i=0
SLEEP_TM=3.5  # why? This is as CONFIG_KCSAN_REPORT_ONCE_IN_MS=3000 by default

# chkconf()
# $1 : string decribing CONFIG_FOO to check
# $1 : CONFIG_FOO to check
chkconf()
{
[ $# -lt 2 ] && return
echo -n "$1:"
grep -q "$2=y" ${KCONF} && echo "enabled" || echo "disabled"
}

# Check for KCSAN support
[ ! -f /sys/kernel/debug/kcsan ] && {
	echo "${name}: kernel requires KCSAN support"
	exit 1
}

# Check for KCSAN assuming plain writes are atomic being disabled
tok=$(chkconf "KCSAN assume plain writes are atomic" CONFIG_KCSAN_ASSUME_PLAIN_WRITES_ATOMIC)
tok2=$(echo ${tok} |awk -F: '{print $2}')
if [ ${tok2} = "enabled" ]; then
	echo "${name}: the kernel config CONFIG_KCSAN_ASSUME_PLAIN_WRITES_ATOMIC is enabled; ideally, you should reconfigure the kernel,
disabling it and then trying this out..."
	exit 1
fi

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

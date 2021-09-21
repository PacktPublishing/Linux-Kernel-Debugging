#!/bin/bash
USE_FTRACE_PR=0
NUM=30
DELAY=3

while [ true ]
do
  [ ${USE_FTRACE_PR} -eq 1 ] && tail -n${NUM} /sys/kernel/debug/tracing/trace || dmesg|tail -n${NUM}
  echo "-----"
  sleep ${DELAY}
done

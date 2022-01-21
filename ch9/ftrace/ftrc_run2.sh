#!/bin/bash
# ch9/ftrace/ftrc_run2.sh
# ***************************************************************
# This program is part of the source code released for the book
#  "Linux Kernel Debugging"
#  (c) Author: Kaiwan N Billimoria
#  Publisher:  Packt
#  GitHub repository:
#  https://github.com/PacktPublishing/Linux-Kernel-Debugging
#
# From: Ch 9: Tracing the kernel flow
#***************************************************************
# Brief Description:
#
# For details, please refer the book, Ch 9.
#------------------------------------------------------------------------------
name=$(basename $0)
[ $(id -u) -ne 0 ] && {
  echo "${name}: needs root."
  exit 1
}

cd /sys/kernel/tracing

echo function_graph > current_tracer
echo 1 > options/funcgraph-proc
echo 1 > options/latency-format

echo 1 > tracing_on ; sleep 1; echo 0 > tracing_on
mkdir -p ~/ftrc 2>/dev/null
cp trace ~/ftrc/ftrc_run2_$(date +%Y%m%d_%H%M%S).txt

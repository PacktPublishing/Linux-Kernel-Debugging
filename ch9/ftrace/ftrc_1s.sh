#!/bin/bash
# ch9/ftrace/ftrc_1s.sh
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
# Very simple (raw) usage of kernel ftrace; traces whatever executes within
# the kernel for 1 second.
#
# For details, please refer the book, Ch 9.
#------------------------------------------------------------------------------
name=$(basename $0)
[ $(id -u) -ne 0 ] && {
  echo "${name}: needs root."
  exit 1
}
source $(dirname $0)/ftrace_common.sh || {
 echo "Couldn't source required file $(dirname $0)/ftrace_common.sh"
 exit 1
}
REPDIR=~/ftrace_reports
FTRC_REP=${REPDIR}/${name}_$(date +%Y%m%d_%H%M%S).txt

cd /sys/kernel/tracing
reset_ftrace

grep -q -w function_graph available_tracers || die "tracer specified function_graph unavailable"
echo function_graph > current_tracer || die "setting function_graph plugin failed"
echo 1 > options/funcgraph-proc
echo 1 > options/latency-format

echo "Tracing with function_graph for 1s ..."
echo 1 > tracing_on ; sleep 1; echo 0 > tracing_on
mkdir -p ${REPDIR} 2>/dev/null
cp trace ${FTRC_REP}
ls -lh ${FTRC_REP}
exit 0

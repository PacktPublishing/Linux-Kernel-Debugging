#!/bin/bash
# ch9/ftrace/ftrc_run3.sh
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
source $(dirname $0)/ftrace_common.sh || {
 echo "Couldn't source required file $(dirname $0)/ftrace_common.sh"
 exit 1
}
REPDIR=~/ftrace_reports

usage() {
 echo "Usage: ${name} function(s)-to-trace
 All available functions are in available_filter_functions.
 You can use globbing; f.e. ${name} kmem_cache*"
}


#--- 'main' here
#[ $# -eq 0 ] && {
#  usage
#  exit 1
#}
[ $# -ge 1 ] && FUNC2TRC="$@"
FTRC_REP=${REPDIR}/ftrc_run3_$(date +%Y%m%d_%H%M%S).txt

cd /sys/kernel/tracing
echo "[+] resetting ftrace"
reset_ftrace

# Tracer
tracer=function_graph
grep -q -w ${tracer} available_tracers || die "tracer specified ${tracer} unavailable"
echo "[+] tracer : ${tracer}"
echo function_graph > current_tracer || die "setting function_graph tracer failed"

#----------- Options -------------------
echo "[+] setting options"
# display the process context
echo 1 > options/funcgraph-proc
# display the name of the terminating function
echo 1 > options/funcgraph-tail
# display the 4 column latency trace info (f.e. dNs1)
echo 1 > options/latency-format
# display the abs time
echo 1 > options/funcgraph-abstime

# buffer size
BUFSZ_PCPU_MB=50
echo "[+] setting buffer size to ${BUFSZ_PCPU_MB} MB / cpu"
echo $((BUFSZ_PCPU_MB*1024)) > buffer_size_kb

# filter?
if [ ! -z "${FUNC2TRC}" ]; then
  grep -q "${FUNC2TRC}" available_filter_functions || die "function(s) specified aren't available for tracing"
  echo "[+] setting set_ftrace_filter"
  echo "${FUNC2TRC}" >> set_ftrace_filter
fi

# filter: only on CPU core 1, i.e., 0000 0010 = 0x2 = 2
#CPUMASK=2
#echo "[+] setting tracing_cpumask to ${CPUMASK}"
#echo ${CPUMASK} > tracing_cpumask 

# filter commands:
# echo '<function>:<command>:<parameter>' > set_ftrace_filter
# try tracing a module
KMOD=e1000
if lsmod|grep ${KMOD} ; then
  echo "[+] setting filter command: !*:mod:${KMOD}"
  echo "!*:mod:${KMOD}" >> set_ftrace_filter
fi

echo "[+] Tracing now ..."
echo 1 > tracing_on ; ping -c3 yahoo.com; echo 0 > tracing_on
#echo 1 > tracing_on ; sleep 1; echo 0 > tracing_on
mkdir -p ${REPDIR} 2>/dev/null
cp trace ${FTRC_REP} || die "report generation failed"
echo "Ftrace report: $(ls -lh ${FTRC_REP})"

exit 0

#!/bin/bash
# ch9/ftrace/ping_ftrace.sh
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
# A quick attempt at tracing a single network ping (initiated with the
# usermode ping(8) utility), using raw ftrace.
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

usage() {
 echo "Usage: ${name} function(s)-to-trace
 All available functions are in available_filter_functions.
 You can use globbing; f.e. ${name} kmem_cache*"
}


#--- 'main' here
[ $# -ge 1 ] && FUNC2TRC="$@"

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
echo > set_ftrace_filter   # reset
if [ ! -z "${FUNC2TRC}" ]; then
  grep -q "${FUNC2TRC}" available_filter_functions || die "function(s) specified aren't available for tracing"
  echo "[+] setting set_ftrace_filter"
  echo "${FUNC2TRC}" >> set_ftrace_filter
fi

# Filter on any network functions: (simplistic)
# 'Inclusive' approach - include and trace only these functions
echo "[+] setting filters for networking funcs only...
 Patience... the string matching can take a while ..."

# Trying to match any string doesn't always work (too big?)
#  'net' for eg., fails... (so does 'sock', 'ip')
#echo " 'net' in available_filter_functions"
#echo $(grep -i net available_filter_functions) >> set_ftrace_filter
echo " 'tcp' in available_filter_functions"
echo $(grep -i tcp available_filter_functions) >> set_ftrace_filter
echo " 'udp' in available_filter_functions"
echo $(grep -i udp available_filter_functions) >> set_ftrace_filter
echo " 'xmit' in available_filter_functions"
echo $(grep -i xmit available_filter_functions) >> set_ftrace_filter

echo "# of functions now being traced: $(wc -l set_ftrace_filter|cut -f1 -d' ')"

# Get rid of unrequired funcs! This is v fast
echo "!*IPI*" >> set_ftrace_filter
echo "!*ipi*" >> set_ftrace_filter
echo "!*ipc*" >> set_ftrace_filter
echo "!*xen*" >> set_ftrace_filter
echo "!*pipe*" >> set_ftrace_filter
echo "!*cipher*" >> set_ftrace_filter
echo "!*chip*" >> set_ftrace_filter
echo "!*__x32*" >> set_ftrace_filter
echo "!*__ia32*" >> set_ftrace_filter
echo "!*__x64*" >> set_ftrace_filter
echo "!*bpf*" >> set_ftrace_filter
#echo "!*selinux*" >> set_ftrace_filter
echo "!*calipso*" >> set_ftrace_filter
echo "!eaf*" >> set_ftrace_filter

echo "# of functions now being traced: $(wc -l set_ftrace_filter|cut -f1 -d' ')"

#---FYI---
# hey, fyi, it's so much more elegant and simple with trace-cmd:
#  sudo trace-cmd record -e net -e sock -F ping -c1 packtpub.com
#  sudo trace-cmd report -l -i trace.dat > reportfile.txt
# The output report here is ~ 4 KB (compared to ~ 574 MB for this raw ftrace report!)
#
# Try https://github.com/kaiwan/trccmd to make it even more flexible!
#---------

# filter commands: put these after all other filtering's done;
# 'a command isn't the same as a filter'!
# echo '<function>:<command>:<parameter>' > set_ftrace_filter
# try tracing a module
KMOD=e1000
if lsmod|grep ${KMOD} ; then
  echo "[+] setting filter command: :mod:${KMOD}"
  echo ":mod:${KMOD}" >> set_ftrace_filter
fi

echo "[+] Tracing now ..."
echo 1 > tracing_on ; ping -c1 packtpub.com; echo 0 > tracing_on
mkdir -p ${REPDIR} 2>/dev/null
cp trace ${FTRC_REP} || die "report generation failed"
echo "Ftrace report: $(ls -lh ${FTRC_REP})"

exit 0

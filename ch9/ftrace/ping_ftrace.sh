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
# An attempt at tracing a single network ping (initiated with the
# usermode ping(8) utility), using raw ftrace.
# It does work but there's still a good deal of 'noise'; the actual ping
# traces seem to be right at the end of the ftrace report. It will be much
# easier and elegant to use trace-cmd (or our wrapper over it, trccmd).
# A choice of filtering is available (though manually), two ways:
#  (a) you can filter either the regular way (via the available_filter_functions
#      pseudofile), or
#  (b) via the set_event interface
# (Ref: https://www.kernel.org/doc/html/v5.10/trace/events.html#via-the-set-event-interface).
# The first gives a much more detailed trace, showing all relevant functions
# (here, the networking ones), but is a little more work to setup.
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
REPDIR=$(pwd)/ftrace_reports
FTRC_REP=${REPDIR}/${name}_$(date +%Y%m%d).txt
#FTRC_REP=${REPDIR}/${name}_$(date +%Y%m%d_%H%M%S).txt
TOP=$(pwd)

usage() {
 echo "Usage: ${name} function(s)-to-trace
 All available functions are in available_filter_functions.
 You can use globbing; f.e. ${name} kmem_cache*"
}

# filterfunc_idx()
# Index-based filtering! *much* faster than the usual string-based filtering.
# Filter only these functions (into set_ftrace_filter) using the index
# position of the function within the available_filter_functions interface.
# Parameters:
#  $1 : function (globs ok) [required]
#  $2 : description string  [optional]
filterfunc_idx()
{
[ $# -lt 1 ] && return
#[ $# -ge 2 ] && echo "$2" || echo " $1 in available_filter_functions"

local func
for func in "$@"
do
  echo $(grep -i -n ${func} available_filter_functions |cut -f1 -d':'|tr '\n' ' ') >> set_ftrace_filter
done
}

# Older string-based filtering; works but is much slower (than above index-based filtering)
filterfunc_str()
{
[ $# -lt 1 ] && return
[ $# -ge 2 ] && echo "$2" || echo " $1 in available_filter_functions"
echo $(grep -i $1 available_filter_functions) >> set_ftrace_filter
echo $(grep -i $1 available_filter_functions) >> set_graph_function
}

# filterfunc_remove()
# Filter to REMOVE these functions
# Parameters:  $1 : function (globs ok) [required]
filterfunc_remove()
{
[ $# -lt 1 ] && return

local func
for func in "$@"
do
  echo "!${func}" >> set_ftrace_filter || echo "removing ${func} failed"
  echo "${func}" >> set_graph_notrace || echo "removing ${func} failed"
done
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
#echo 1 > options/funcgraph-tail
# display the 4 column latency trace info (f.e. dNs1)
echo 1 > options/latency-format
# display the timestamp
echo 1 > options/funcgraph-abstime

# Can limit the depth
#echo 3 > max_graph_depth
# Stack trace to see max stack utilization
#echo 1 > /proc/sys/kernel/stack_tracer_enabled

#--- per cpu buffer size
# Try and use 5% of available memory
# TODO/RELOOK:
# Careful! This isn't well tested... if the amt of RAM isn't v large but the
# number of CPU cores is, it can lead to problems
PERCENT2USE=5
AVAILMEM_KB=$(grep "^MemAvailable" /proc/meminfo |awk '{print $2}')
BUFSZ_PCPU_KB=$((${AVAILMEM_KB}*${PERCENT2USE}/100))
BUFSZ_PCPU_MB=$((${BUFSZ_PCPU_KB}/1024))
[ ${BUFSZ_PCPU_MB} -le 1 ] && BUFSZ_PCPU_MB=2
echo "[+] setting buffer size to ${BUFSZ_PCPU_MB} MB / cpu"
echo $((BUFSZ_PCPU_MB*1024)) > buffer_size_kb

#---------------------- Function Filtering ------------------------------------
# Any specific funcs to trace?
echo > set_ftrace_filter   # reset
if [ ! -z "${FUNC2TRC}" ]; then
  grep -q "${FUNC2TRC}" available_filter_functions || die "function(s) specified aren't available for tracing"
  echo "[+] setting set_ftrace_filter"
  echo "${FUNC2TRC}" >> set_ftrace_filter
  echo "${FUNC2TRC}" >> set_graph_function
fi

#--- Filtering of functions:
# Can be done in one of two ways here:
# a) via the regular available_filter_functions pseudofile interface, or
# b) via the set_event pseudofile interface
# They're mutually exclusive;if you set FILTER_VIA_AVAIL_FUNCS to 1, then we
# filter via (a), else via (b).
# The first gives a much more detailed trace, showing all relevant functions
# (here, the networking ones), but needs a little more work (index-based filtering)
# to keep it quick.
FILTER_VIA_AVAIL_FUNCS=1

echo "[+] Function filtering:"
if [ ${FILTER_VIA_AVAIL_FUNCS} -eq 1 ] ; then
 # Filter on any network functions: (simplistic)
 # 'Inclusive' approach - include and trace only these functions
 echo " Regular filtering (via available_filter_functions):
 Setting filters for networking funcs only..."
 # This is pretty FAST and yields good detail!
 filterfunc_idx read write net packet_ sock sk_ tcp udp skb netdev \
   netif_ napi icmp "^ip_" "xmit$" dev_ qdisc

else # filter via the set_event interface

 # This is fast but doesn't yield as good detail!
 echo " Alternate event-based filtering (via set_event):"
 echo 'net:*' >> set_event
 echo 'sock:*' >> set_event
 echo 'skb:*' >> set_event
 echo 'tcp:*' >> set_event
 echo 'udp:*' >> set_event
 echo 'napi:*' >> set_event
 echo 'qdisc:*' >> set_event
 echo 'syscalls:*' >> set_event
fi

#--- Get rid of unrequired funcs! This is very fast
# NOTE: depending on your particular kernel ver and config, this list of funcs
# to remove can vary.
echo "[+] filter: remove unwanted functions"
filterfunc_remove "*idle*" "tick_nohz_idle_stop_tick" "rcu_*" "*__rcu_*" \
  "*down_write*" "*up_write*" "*down_read*" "*up_read*" \
  "*get_task_policy*" "*kthread_blkcg*" "*kthread_blkcg*" \
  "*IPI*" "*ipi*" "*ipc*" "*xen*" "*pipe*" "*cipher*" "*chip*" "*__x32*" \
  "*vma*" "*__ia32*" "*__x64*" "*bpf*" "*calipso*" "eaf*" "setup_object_debug*" \
  "*alloc*" "*fail_alloc*" "*get_page*" "*prep_new*" "*numa*" "*kmem_*" "vm_*" \
  "*copy_page*" "*memcg*" "tick_*" "handle_mm*" "task_work*"
  #"*selinux*"

echo "# of functions now being traced: $(wc -l set_ftrace_filter|cut -f1 -d' ')"

# filter commands: put these after all other filtering's done;
# 'a command isn't the same as a filter'!
# echo '<function>:<command>:<parameter>' > set_ftrace_filter
# try tracing a module
KMOD=e1000
echo "[+] module filtering (for ${KMOD})"
if lsmod|grep ${KMOD} ; then
  echo "[+] setting filter command: :mod:${KMOD}"
  echo ":mod:${KMOD}" >> set_ftrace_filter
fi

#--- Running the target program and tracing it --------------------------------
echo "[+] Setting up wrapper runner process now..."
CMD="ping -c1 packtpub.com"

# Keep these vars in sync with the 'runner' script!
TRIGGER_FILE=/tmp/runner
CPUMASK=2

${TOP}/runner ${CMD} &
PID=$(pgrep --newest runner)
[ -z "${PID}" ] && {
   rm -f ${TRIGGER_FILE}
   pkill runner
   die "Couldn't get PID of runner wrapper process"
}

# Filter by PID and CPU (1)
# trace only what this process (and it's children) do
echo 0 > set_ftrace_notrace_pid
echo function-fork >> trace_options  # trace any children as well
[ ${FILTER_VIA_AVAIL_FUNCS} -eq 1 ] && echo ${PID} > set_ftrace_pid || \
   echo ${PID} > set_event_pid
echo ${CPUMASK} > tracing_cpumask

touch ${TRIGGER_FILE} # doing this triggers the command and it runs

#pwd
echo "[+] Tracing PID ${PID} on CPU 1 now ..."
echo markers > trace_options
echo 1 > tracing_on
 # So, whatever happens here in the kernel gets traced; thus, it's not
 # completely exclusive to only our process of interest; other stuff can get
 # caught in the trace...
 # Using a *trace marker* (as below) is very useful! We can search for the string
 # in the trace report and figure where the interesting portion actually is!
 #---
 # RELOOK:
 # Pecuiliar: writing to the trace_marker pseudofile fails with
 #  bash: echo: write error: Bad file descriptor
 # on my VMs, but work fine on my native Linux system...
 #---
echo START > trace_marker
wait ${PID}
echo END > trace_marker
echo 0 > tracing_on
rm -f ${TRIGGER_FILE}

# Older way: doing it this way, we seem to miss the ping itself!
#echo 1 > tracing_on ; ping -c1 packtpub.com; echo 0 > tracing_on

mkdir -p ${REPDIR} 2>/dev/null
cp -f trace ${FTRC_REP} || die "report generation failed"

# Make the report simpler to scan by only keeping lines relevant to the
# ping-${PID} process !
egrep "ping-${PID}|START|END" trace > ${REPDIR}/ping_only_$(date +%Y%m%d).txt

echo "Ftrace reports:"
ls -lh ${FTRC_REP} ${REPDIR}/ping_only_$(date +%Y%m%d).txt

#---FYI---
# hey, think on this, it's so much simpler with trace-cmd(1):
#  sudo trace-cmd record -e net -e sock -F ping -c1 packtpub.com
#  sudo trace-cmd report -l -i trace.dat > reportfile.txt
#
# Even simpler, use our wrapper over trace-cmd: https://github.com/kaiwan/trccmd
# We do the same with our trccmd wrapper util (ch9/tracecmd/trc-cmd*.sh).
#---------
exit 0

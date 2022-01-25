#!/bin/bash
# ch9/ftrace/ftrace_common.sh
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

die()
{
 echo "$@" 1>&2
 exit 1
}

reset_ftrace()
{
local f

echo 1408 > buffer_size_kb  # 1408 KB is the default (5.10)

# wipe out any existing trace data
echo > trace

# Tip: do NOT attempt to reset tracing_cpumask by 'echo > tracing_cpumask';
# Causes trace to fail... as a value of 0x0 as cpu bitmask effectively disables
# tracing!

for f in set_ftrace_filter set_ftrace_notrace set_ftrace_notrace_pid set_ftrace_pid
do
 echo "resetting $f"
 echo > $f 
done

# trace_options to defaults (as of 5.10.60)
echo "resetting trace_options to defaults (as of 5.10.60)"
echo print-parent > trace_options 
echo nosym-offset > trace_options
echo nosym-addr > trace_options
echo noverbose > trace_options
echo noraw > trace_options
echo nohex > trace_options
echo nobin > trace_options
echo noblock > trace_options
echo trace_printk > trace_options
echo annotate > trace_options
echo nouserstacktrace > trace_options
echo nosym-userobj > trace_options
echo noprintk-msg-only > trace_options
echo context-info > trace_options
echo nolatency-format > trace_options
echo record-cmd > trace_options
echo norecord-tgid > trace_options
echo overwrite > trace_options
echo nodisable_on_free > trace_options
echo irq-info > trace_options
echo markers > trace_options
echo noevent-fork > trace_options
echo nopause-on-trace > trace_options
echo function-trace > trace_options
echo nofunction-fork > trace_options
echo nodisplay-graph > trace_options
echo nostacktrace > trace_options
echo notest_nop_accept > trace_options
echo notest_nop_refuse > trace_options

# options/funcgraph-*  to defaults
echo "resetting options/funcgraph-*"
echo 0 > options/funcgraph-abstime
echo 1 > options/funcgraph-cpu
echo 1 > options/funcgraph-duration
echo 1 > options/funcgraph-irqs
echo 1 > options/funcgraph-overhead
echo 0 > options/funcgraph-overrun
echo 1 > options/funcgraph-proc
echo 0 > options/funcgraph-tail

# perf-tools ftrace reset script 
f=$(which reset-ftrace-perf)
[ ! -z "$f" ] && {
  echo "running '$f -q' now..."
  $f -q
}
}

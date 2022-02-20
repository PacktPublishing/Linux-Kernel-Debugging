#!/bin/bash
# ch9/trace-cmd/trc-cmd1.sh
# Simple wrapper over trace-cmd
[ $# -ne 1 ] && {
  echo "Usage: $0 report-file"
  exit 1
}
runcmd()
{
    [ $# -eq 0 ] && return
    echo "$@"
    eval "$@"
}
rep=$1

#--- RECORD
runcmd sudo trace-cmd record -p function_graph -e net -e sock -e skb -e tcp -e udp -F ping -c1 packtpub.com

#--- REPORT
[ -f $1 ] && mv -f $1 $1.bkp
runcmd "sudo trace-cmd report -l  > $1"
# Typically, the report file is still fairly large (~ 9 to 10 MB in my tests)
# as all related kernel functions show up in the trace.

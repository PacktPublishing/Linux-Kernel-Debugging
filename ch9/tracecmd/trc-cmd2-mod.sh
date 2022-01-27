#!/bin/bash
# ch9/trace-cmd/trc-cmd1.sh
# Simple wrapper over trace-cmd; same as trc-cmd1.sh except that here,
# we ALSO trace only the e1000 (network driver) module!
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

runcmd sudo trace-cmd record -p function_graph -e net -e sock --module e1000 -F ping -c1 packtpub.com

[ -f $1 ] && mv -f $1 $1.bkp
runcmd sudo trace-cmd report -l  > $1
# Typically, the report file is now pretty tiny (~ 4 to 5 KB in my tests)
# as ONLY the e1000 related module/kernel functions show up in the trace.

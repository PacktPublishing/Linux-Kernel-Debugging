#!/bin/bash
# ch9/trace-cmd/trc-cmd1.sh
# Simple wrapper over trace-cmd; same as trc-cmd1.sh except that here,
# we trace only the e1000 (network driver) module!
[ $# -ne 1 ] && {
  echo "Usage: $0 report-file"
  exit 1
}
die() {
  echo $@ >&2
  exit 1
}
KMOD=e1000

runcmd()
{
    [ $# -eq 0 ] && return
    echo "$@"
    eval "$@"
}
rep=$1

lsmod|grep ${KMOD} || die "Module ${KMOD} isn't loaded"
runcmd sudo trace-cmd record -q -p function_graph -e net -e sock -e skb -e tcp -e udp \
 --module e1000 -F ping -c1 packtpub.com

[ -f $1 ] && mv -f $1 $1.bkp
runcmd sudo trace-cmd report -q -l  > $1
# Typically, the report file is now pretty tiny (~ 4 to 5 KB in my tests)
# as ONLY the e1000 related module/kernel functions show up in the trace.

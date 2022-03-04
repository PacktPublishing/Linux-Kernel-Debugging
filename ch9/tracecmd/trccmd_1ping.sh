#!/bin/bash
# ch9/trace-cmd/trc-cmd1.sh
# Simple wrapper over trace-cmd
usage() {
  echo "Usage: $0 option
option = -f : show in function_graph format
option = -p : show the parameters"
  exit 1
}
runcmd()
{
    [ $# -eq 0 ] && return
    echo "$@"
    eval "$@"
}

#--- 'main'
[ $(id -u) -ne 0 ] && {
  echo "$0: needs root."
  exit 1
}
[ $# -ne 1 ] && {
  usage; exit 1
}

SHOW_FUNCGRAPH=0
SHOW_PARAMS=0
case "$1" in
 -f ) SHOW_FUNCGRAPH=1 ;;
 -p ) SHOW_PARAMS=1 ;;
 *) usage ; exit 1 ;;
esac
REP=ping_trccmd.txt
cmd="ping -c1 packtpub.com"
events="-e net -e sock -e skb -e tcp -e udp"

#--- RECORD
if [ ${SHOW_FUNCGRAPH} -eq 1 ] ; then
  runcmd trace-cmd record -p function_graph ${events} -F ${cmd}
elif [ ${SHOW_PARAMS} -eq 1 ] ; then
  runcmd trace-cmd record ${events} -F ${cmd}
fi

#--- REPORT
[ -f ${REP} ] && mv -f ${REP} ${REP}.bkp
trace-cmd report -l > ${REP}
#  -l : show 'latency-format' columns
# Typically, the report file is still fairly large (~ 9 to 10 MB in my tests)
# as all related kernel functions show up in the trace.

exit 0

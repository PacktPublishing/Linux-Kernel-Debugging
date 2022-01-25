#!/bin/bash
# ch9/trace-cmd/trc-cmd1.sh
# Simple wrapper over trace-cmd

[ $# -ne 1 ] && {
  echo "Usage: $0 report-file"
  exit 1
}
rep=$1

#--- RECORD
echo "sudo trace-cmd record -p function_graph -e net -e sock -F ping -c1 packtpub.com"
sudo trace-cmd record -p function_graph -e net -e sock -F ping -c1 packtpub.com

#--- REPORT
[ -f $1 ] && mv -f $1 $1.bkp
echo "sudo trace-cmd report -l  > $1"
sudo trace-cmd report -l  > $1
# Typically, the report file is still fairly large (~ 9 to 10 MB in my tests)
# as all related kernel functions show up in the trace.

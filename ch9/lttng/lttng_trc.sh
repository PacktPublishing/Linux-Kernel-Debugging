#!/bin/bash
# lttng_trc.sh
#
# A simple frontend to using the powerful kernel-space LTTng tracer.
# Note: this script does only the (auto) kernel-space tracing, usermode tracing
# has to be setup by appropriately instrumenting the app w/ tracepoints.
#
# Once this tracing is done, run the TraceCompass GUI, open and analyze
# the trace.
#
# TODO:
#  - event filters
#  - time traced
name=$(basename $0)

die()
{
	echo >&2 " *** Fatal ***
$@"
	exit 1
}

SESSION=lttng
TRC_OUT_DIR=/tmp

trap 'finish_trace' INT QUIT

finish_trace()
{
	echo "[+] cleaning up..."
USR=$(who |head -n1 |awk '{print $1}')
chown -R ${USR}:${USR} ${TRC_OUT_DIR}
echo "${name}: done. Trace files in ${TRC_OUT_DIR} ; size: "
du -ms ${TRC_OUT_DIR}
lttng destroy

echo " [+] ...generating compressed tar file of trace now, pl wait ..."
TARBALL=${SESSION}.tar
tar -cf ${TARBALL} ${TRC_OUT_DIR}/
gzip -9 ${TARBALL}
ls -lh ${TARBALL}*
}

#--- "main" here
which lttng >/dev/null || die "lttng not installed?"
[ $(id -u) -ne 0 ] && die "need to be root."

[ $# -lt 2 ] && {
 echo "Usage: ${name} session-name program-to-trace-with-LTTng|0
  1st parameter: name of the session
  2nd parameter, ...:
    If '0' is passed, we just do a trace of the entire system (all kevents),
    else we do a trace of the particular process (all kevents).
 Eg. sudo ./${name} ps_trace ps -LA
 [NOTE: other stuff running _also_ gets traced (this is non-exclusive)]."
 exit 3
}

echo "Session name :: \"$1\""
echo -n "[+] (Minimal) Checking for LTTng support ... "
lsmod |grep -q "^lttng_" || die "lttng kernel modules not seen"
echo "[OK]"

# 1. Set up a recording session
# TODO - put the full name instead of just $1;
# but, will need to replace spaces with an '_'
SESSION=${SESSION}_$1_$(date +%d%b%y_%H%M)
TRC_OUT_DIR=${TRC_OUT_DIR}/${SESSION}
echo "[+] lttng create ${SESSION} --output=${TRC_OUT_DIR}"
lttng create ${SESSION} --output=${TRC_OUT_DIR} || die "lttng create failed"

# 2. Set up kernel events to record; simplistic: record all
# WARNING! Big trace files with ALL kernel events
echo "[+] lttng enable events ..."
lttng enable-event --kernel --all
# userspace tracef() [??]
lttng enable-event --userspace 'lttng_ust_tracef:*'

# 3. Perform the trace
if [ "$2" = "0" ]; then
  echo "@@@ ${name}: Tracing system now ... press [Enter] or ^C to stop! @@@"
  date ; date +%s.%N   # timestamp
  lttng start
  read
else
  #echo "params: num=$# val=$@"
  shift # $1 is the session name; $2 onward is the program to execute
  echo "@@@ ${name}: Tracing \"$@\" now ... @@@"
  date ; date +%s.%N   # timestamp
  lttng start ; eval "$@" ; lttng stop
  date ; date +%s.%N   # timestamp
fi

date ; date +%s.%N   # timestamp
# 4. Stop recording, destroy the session, ...
finish_trace
exit 0

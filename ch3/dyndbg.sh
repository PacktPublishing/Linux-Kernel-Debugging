#!/bin/bash
# dyndbg
# Simple frontend to the kernel's Dynamic Debug facility
# (c) 2021, Kaiwan NB
# License: MIT
name=$(basename $0)
SEP="------------------------------------------------------------------------"

get_control_file()
{
local ctrlfile
mount|grep -q -w debugfs
if [ $? -eq 0 ]; then
  local dbgfs_mnt=$(mount|grep -w debugfs|awk '{print $3}')
  [ -z "${dbgfs_mnt}" ] && return -1
  ctrlfile="${dbgfs_mnt}/dynamic_debug/control"
else
  ctrlfile="/proc/dynamic_debug/control"
fi
echo "${ctrlfile}"
}

# show_dbgpr_on()
# Params:
# $1 : dyn debug control file
show_dbgpr_on()
{
[ $# -lt 1 ] && return
# first line is the format-spec
local num_on=$(grep -v " =_ " ${1} |sed '1d' |wc -l)
if [ ${num_on} -le 0 ]; then
  echo "No dynamic debug prints are currently enabled"
  return
fi
echo "${SEP}
${num_on} dynamic debug prints are currently enabled
${SEP}"
grep -v " =_ " ${1}
}

# show_dbgpr_off()
# Params:
# $1 : dyn debug control file
show_dbgpr_off()
{
[ $# -lt 1 ] && return
# first line is the format-spec
local num_on=$(grep -v " =_ " ${1} |sed '1d' |wc -l)
if [ ${num_on} -le 0 ]; then
  echo "No dynamic debug prints are currently enabled"
  return
fi
echo "${SEP}
${num_on} dynamic debug prints are currently enabled
${SEP}"
grep " =_ " ${1}
}

#--- 'main' ---

if [ $(id -u) -ne 0 ] ; then
   echo "${name}: needs root."
   exit 1
fi

CTRLFILE=$(get_control_file)
show_dbgpr_on ${CTRLFILE}

exit 0

#!/bin/sh
#------------------------------------------------------------------
# err_common.sh
#
# Common error handling routines.
# 
# (c) Kaiwan N Billimoria
# kaiwan -at- designergraphix -dot- com
# GPL / LGPL
# Last Updt: 31Mar2014
#------------------------------------------------------------------

GUI_MODE=1
export TOPDIR=$(pwd)
ON=1
OFF=0

# "Returns" (actually echoes) 
#   1 => zenity present
#   0 => zenity absent
verify_zenity()
{
 which zenity >/dev/null 2>&1 && echo -n 1 || echo -n 0
}

# QP
# QuickPrint ;-)
# Print timestamp, script name, line#. Useful for debugging.
QP()
{
	_ERR_HDR_FMT="%.23s %s[%s]: "
	_ERR_MSG_FMT="${_ERR_HDR_FMT}%s\n"
	printf "$_ERR_MSG_FMT" $(date +%F.%T.%N) ${BASH_SOURCE[1]##*/} ${BASH_LINENO[0]} 1>&2  #"${@}"
	unset _ERR_HDR_FMT
	unset _ERR_MSG_FMT
}


cli_handle_error()
{
  echo -n "FatalError :: " 1>&2
  QP
  [ $# -lt 1 ] && exit -1
  echo "${@}" 1>&2
  exit -1
}

# FatalError
# Display the error message string and exit -1.
# Parameter(s):
#  $1 : Error string to display [required]
# Returns: -1 (255)
FatalError()
{
	[ ${GUI_MODE} -eq 0 ] && {
	  cli_handle_error $@
	} || {   # want GUI mode
	  #n=$(verify_zenity)
	  #echo "n=$n"
	  #return
	  [ $(verify_zenity) -eq 0 ] && {
	    echo "FatalError :: !WARNING! zenity not installed?? "
		# fallback to non-gui err handling
	    cli_handle_error $@
	  }
	}
	# gui err handling, zenity there; whew
	zenity --error --title="${name}: Error" --text="Fatal Error :: $@"
    cli_handle_error $@
	exit -1
}

# Prompt
# Interactive: prompt the user to continue by pressing ENTER or
# abort by pressing Ctrl-C
# Parameter(s):
#  $1 : string to display (string)
Prompt()
{
	[ $# -lt 1 ] && {
	  echo "$0: Prompt function requires a string parameter!"
	  return 1
	}
	echo "${@}"
	echo " Press ENTER to continue, Ctrl-C to abort now..."
	read
}



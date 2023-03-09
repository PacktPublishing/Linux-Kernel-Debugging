#!/bin/bash
# ch4/kprobes/4_kprobe_helper/kp_load.sh
# ***************************************************************
# This program is part of the source code released for the book
#  "Linux Kernel Debugging"
#  (c) Author: Kaiwan N Billimoria
#  Publisher:  Packt
#  GitHub repository:
#  https://github.com/PacktPublishing/Linux-Kernel-Debugging
#
# From: Ch 4: Debug via Instrumentation - Kprobes
#***************************************************************
# Brief Description:
# Our kprobes demo #4:
# Traditional, semi-automated manual approach: a helper script generates a
# template for both the kernel module C code and the Makefile, enabling
# attaching a kprobe to a given function via module parameter.
# (See the 'usage' screen by just typing the script's name with no arguments).
# 
# Notes:
# - The function to kprobe must not be marked 'static' or 'inline' in the kernel
#   / LKM or be blacklisted by the kprobes machinery
# - This script dynamically "generates" an LKM in <pwd>/tmp/ .
# Filename format: ${BASEFILE}-${FUNCTION}-$(date +%d%m%y).ko
# To do so, it generates a Makefile and builds the LKM, and even insmod's it!
#
# Author: Kaiwan N Billimoria
# License: MIT
#------------------------------------------------------------------------------
name=$(basename $0)
source ./common.sh || {
 echo "$name: could not source common.sh , aborting..."
 exit 1
}

# Function to validate passed as first parameter
check_function()
{
local FUNC=$1
if [ -z "${FUNC}" ]; then
	echo
	echo "*** ${name}: function name null, aborting now.."
	exit 1
fi
ShowTitle "[ Validate the to-be-kprobed function ${FUNC} ]"

# Attempt to find out if it's valid in the kernel.
# In any case, if the function is invalid, it will be caught on the 
# register_kprobe(), which will then fail..

if [ ! -f /proc/kallsyms ]; then
  if [ ! -f /boot/System.map-$(uname -r) ]; then
  	echo
    echo "$name: WARNING! Both /proc/kallsyms and /boot/System.map-$(uname -r) not present!?
[Possibly an embedded system]. 
So, we'll Not attempt to check validity of ${FUNC} right now; if invalid, it will 
be subsequently caught in the register_kprobe().
	"
	return
  fi
fi

if [ -f /proc/kallsyms ]; then
  SYMLOC=/proc/kallsyms
elif [ -f /boot/System.map-$(uname -r) ]; then
  SYMLOC=/boot/System.map-$(uname -r)
else
  # TODO: what about embedded system which don't have either SYMLOC ??
  SYMLOC=${DBGFS_MNT}/tracing/available_filter_functions  # ??
fi
#echo "SYMLOC=${SYMLOC}"

# Is the to-be-kprobe'd function blacklisted?
if [ ! -z "${DBGFS_MNT}" ]; then
 if [ -d ${DBGFS_MNT} ]; then
   grep -q -w "${FUNC}" ${DBGFS_MNT}/kprobes/blacklist && {
   echo "
*** ${name}: FATAL: the symbol '${FUNC}' is blacklisted by the Kprobes
framework. Aborting..."
   exit 1
  }
 fi
fi

grep -w "[tT] ${FUNC}" ${SYMLOC} || {
 echo "
*** ${name}: FATAL: Symbol '${FUNC}' not found!
[Either it's invalid -or- Could it be static or inline?]. Aborting..."

 echo "--- Possible close matches ---"
 grep "${FUNC}" /proc/kallsyms |awk '{print $3}'|grep -v "\."
 exit 1
 }
num=$(grep -w "[tT] ${FUNC}" ${SYMLOC} |wc -l)
[ ${num} -gt 1 ] && {
 echo "
 ### $name: WARNING! Symbol '${FUNC}' - multiple instances found!
"
 }
}

# Insert the helper_kp kernel module that will set up our custom kprobe
load_helperkp_module()
{
 echo "/sbin/insmod ./${KPMOD}.ko funcname=${FUNCTION} verbose=${VERBOSE} show_stack=${SHOWSTACK}"
 /sbin/insmod ./${KPMOD}.ko funcname=${FUNCTION} verbose=${VERBOSE} show_stack=${SHOWSTACK} || {
	echo "${name}: insmod ${KPMOD} unsuccessful, aborting now.."
	if [ ${PROBE_KERNEL} -eq 0 ]; then
		/sbin/rmmod ${TARGET_MODULE} 2>/dev/null
	fi
	echo "dmesg|tail"
	dmesg|tail
	exit 7
 }
}

# If not already inserted, insert the LKM (kernel module) ${KPMOD}
# Running as root here...
insert_kprobe()
{
if [ ${PROBE_KERNEL} -eq 0 ] ; then
 local already_inserted=0
 local kmod_name=$(basename ${TARGET_MODULE::-3})  # rm the .ko too...
 lsmod|grep -w ${kmod_name} >/dev/null && already_inserted=1

# echo "+++ already_inserted=${already_inserted}"

 dmesg -C
 echo ${SEP}
 if [ ${already_inserted} -eq 0 ]; then
    echo " Inserting target kernel module ${TARGET_MODULE} now..."
    # If a module function is to be probed, first insert the kernel module
	/sbin/insmod ${TARGET_MODULE} || {
		echo "$name: insmod ${TARGET_MODULE} unsuccessful, aborting now.."
		echo "dmesg|tail"
		dmesg|tail
		exit 5
	}
	echo "${name}: insmod ${TARGET_MODULE} successful."
	echo "dmesg|tail"
	dmesg|tail
 else
    echo " kernel module ${KPMOD} is already inserted... proceeding..."
 fi
 load_helperkp_module
else # probing a kernel func..
 load_helperkp_module
 echo "${name}: successful."
 echo "dmesg|tail"
 dmesg|tail
 cd ..
fi
}

usage()
{
	echo "Usage: ${name} [--verbose] [--help] [--mod=module-pathname] --probe=function-to-probe
       ---probe=probe-this-function  : if module-pathname is not passed, 
                                           then we assume the function to be kprobed is in the kernel itself.
       [--mod=module-pathname]       : pathname of kernel module that has the function-to-probe
       [--verbose]                   : run in verbose mode; shows PRINT_CTX() o/p, etc
       [--showstack]                 : display kernel-mode stack, see how we got here!
       [--help]                      : show this help screen"
	exit
}

kprobes_check()
{
echo -n "[+] Performing basic sanity checks for kprobes support... "
KPROBES_SUPPORTED=0
[ -f /boot/config-$(uname -r) ] && {
	grep -w -q "CONFIG_KPROBES=y" /boot/config-$(uname -r) && KPROBES_SUPPORTED=1
}
[ -f /boot/System.map-$(uname -r) ] && {
	grep -q -w register_kprobe /boot/System.map-$(uname -r) && KPROBES_SUPPORTED=1
}
modprobe configs 2>/dev/null
[ -f /proc/config.gz ] && {
	zcat /proc/config.gz |grep -i kprobes >/dev/null && KPROBES_SUPPORTED=1
}
[ ${KPROBES_SUPPORTED} -eq 0 ] && {
	  echo "${name}: Kprobes does not seem to be supported on this kernel [2]."
	  exit 1
}
echo " OK
"
}


### "main" here ###

# PROBE_KERNEL=0 : we're going to attempt to kprobe a function in a kernel module
#  In this case (the default), the script's first parameter is the kernel module.
# PROBE_KERNEL=1 : we're going to attempt to kprobe a function in the kernel itself
#  In this case, the script's first parameter will not be a kernel module pathname
#  and we shall accordingly treat the first parameter as the name of the function
#  to kprobe.
PROBE_KERNEL=1

SEP="-------------------------------------------------------------------------------"
name=$(basename $0)

if [ $(id -u) -ne 0 ]; then
	echo "${name}: requires root."
	exit 1
fi
mount|grep -q -w debugfs
if [ $? -eq 0 ]; then
  DBGFS_MNT=$(mount|grep -w debugfs|awk '{print $3}')
fi
kprobes_check

VERBOSE=0
SHOWSTACK=0
optspec=":h?-:"
while getopts "${optspec}" opt
do
    #echo "1. opt = ${opt}  ind=${OPTIND}  OPTARG = ${OPTARG}"
    case "${opt}" in
		-)                 # 'long' opts '--xxx' style, ala checksec!
        #echo "1. opt = ${opt}  ind=${OPTIND}  OPTARG = ${OPTARG}"
		    case "${OPTARG}" in
			  h|?|help)
			    usage
				exit 0
				;;
			  probe=*) #echo "--probe ! p: $@; ind=$OPTIND arg= $OPTARG" 
				FUNCTION=$(echo "${OPTARG}" |cut -d'=' -f2) ;;
			  mod=*) #echo "--mod ! p: $@; ind=$OPTIND arg= $OPTARG" 
				TARGET_MODULE=$(echo "${OPTARG}" |cut -d'=' -f2)
				PROBE_KERNEL=0 ;;
			  verbose) VERBOSE=1 ;;
			  showstack) SHOWSTACK=1 ;;
			  *) echo "Unknown option '${OPTARG}'" #; usage
				;;
  	        esac
  	esac
done
shift $((OPTIND-1))

[ ${VERBOSE} -eq 1 ] && echo "FUNCTION=${FUNCTION} PROBE_KERNEL=${PROBE_KERNEL} TARGET_MODULE=${TARGET_MODULE} ; VERBOSE=${VERBOSE} SHOWSTACK=${SHOWSTACK}"
[ -z "${FUNCTION}" ] && {
  echo "${name}: minimally, a function to be kprobe'd has to be specified (via the --probe=func option)
"
  usage
}
echo -n "Verbose mode is "
[ ${VERBOSE} -eq 1 ] && echo "on" || echo "off"

check_function ${FUNCTION}

if [ ${PROBE_KERNEL} -eq 0 ]; then
	if [ ! -f ${TARGET_MODULE} ]; then
		echo "${name}: kernel module pathname '${TARGET_MODULE}' that you passed seems to be an invalid pathname, aborting now.."
		exit 1
	fi
	echo "Target kernel Module: ${TARGET_MODULE}"
fi

################ Generate a kernel module to probe this particular function ###############
BASEFILE_C=helper_kp.c
BASEFILE_H=../../../convenient.h
BASEFILE=helper_kp

if [ ! -f ${BASEFILE_C} ]; then
  echo "${name}: base file ${BASEFILE_C} missing?"
  exit 1
fi
if [ ! -f ${BASEFILE_H} ]; then
  echo "${name}: base file ${BASEFILE_H} missing?"
  exit 1
fi

export KPMOD=${BASEFILE}-${FUNCTION}-$(date +%d%b%y)
#export KPMOD=${BASEFILE}-${FUNCTION}-$(date +%d%m%y_%H%M%S)
echo $SEP
echo "KPMOD=${KPMOD}"

rm -rf tmp/ 2>/dev/null
mkdir -p tmp/
cd tmp
cp ../${BASEFILE_C} ${KPMOD}.c || exit 1

echo "--- Generating tmp/Makefile ---------------------------------------------------"
# Generate the Makefile
cat > Makefile << @MYMARKER@
# Makefile for kernel module
### Specifically: Kprobe helpers !

ifneq (\$(KERNELRELEASE),)
  \$(info --- Dynamic Makefile for helper_kprobes util ---)
  \$(info Building with KERNELRELEASE = ${KERNELRELEASE})
  # If you choose to keep the define USE_FTRACE_PRINT , we'll use
  # trace_printk() , else the regular printk()
  EXTRA_CFLAGS += -DDEBUG  # use regular pr_*()
  obj-m += ${KPMOD}.o

else
	#########################################
	# To support cross-compiling for the ARM:
	# For ARM, invoke make as:
	# make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- 
	ifeq (\$(ARCH),arm)
	# Update 'KDIR' below to point to the ARM Linux kernel source tree
		KDIR ?= ~/5.4
	else
		KDIR ?= /lib/modules/\$(shell uname -r)/build 
	endif
	#########################################
	PWD   := \$(shell pwd)
default:
	\$(MAKE) -C \$(KDIR) M=\$(PWD) modules
install:
	\$(MAKE) -C \$(KDIR) M=\$(PWD) modules_install
endif
clean:
	\$(MAKE) -C \$(KDIR) SUBDIRS=\$(PWD) clean
@MYMARKER@

echo "--- make ---------------------------------------------------"
make || {
  echo "${name}: failed to 'make'. Aborting..."
  cd ..
  exit 1
}

ls -l ${KPMOD}.ko
insert_kprobe

exit 0

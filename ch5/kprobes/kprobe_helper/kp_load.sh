#!/bin/bash
#------------------------------------------------------------------------------
# kp_load.sh
# Helper script for the Kprobes Helper kernel module.
# Params:
#  $1 : Kernel Module pathname (func to kprobe is in this LKM) [OPTIONAL]
#  $2 : Function name to kprobe [REQD]
#  $3 : Verbose flag [0|1] [OPTIONAL]
# 
# Notes:
# - The function to kprobe must not be marked 'static' or 'inline' in the kernel / LKM.
# - This script dynamically "generates" an LKM in <pwd>/tmp/ .
# Filename format: ${BASEFILE}-${FUNCTION}-$(date +%d%m%y_%H%M%S).ko
# To do so, it generates a Makefile and builds the LKM, and even insmod'd it!
#
# Author: Kaiwan N Billimoria <kaiwan@designergraphix.com>
# License: [L]GPL
#------------------------------------------------------------------------------
name=$(basename $0)
source ./common.sh || {
 echo "$name: could not source common.sh , aborting..."
 exit 1
}

# Function to validate passed as first parameter
check_function()
{
FUNC=$1
if [ -z "${FUNC}" ]; then
	echo
	echo "*** ${name}: function name null, aborting now.."
	exit 1
fi
ShowTitle "[ Validate the to-be-Kprobed function ${FUNC} ]"

# Attempt to find out if it's valid in the kernel.
# In any case, if the function is invalid, it will be caught on the 
# register_kprobe(), which will then fail..

# TODO: what about embedded system which don't have either SYMLOC ??
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
fi

grep -w "[tT] ${FUNC}" ${SYMLOC} || {
 echo
 echo "*** $name: FATAL: Symbol '${FUNC}' not found!
 [Either it's invalid -or- Could it be static or inline?]. Aborting..."
 exit 1
 }
num=$(grep -w "[tT] ${FUNC}" ${SYMLOC} |wc -l)
[ ${num} -gt 1 ] && {
 echo "
 ### $name: WARNING! Symbol '${FUNC}' - multiple instances found!
"
 #exit 1
 }
}

# Running as root here...
insert_kprobe()
{
########## Lets run! #########################
echo ${SEP}
# rmmod any old instances
/sbin/rmmod ${KPMOD} 2>/dev/null
if [ ${PROBE_KERNEL} -eq 0 ]; then
	/sbin/rmmod ${TARGET_MODULE} 2>/dev/null
fi

dmesg -C

# 1. If a module function is to be probed, first insert the kernel module
if [ ${PROBE_KERNEL} -eq 0 ]; then
	/sbin/insmod ${TARGET_MODULE} || {
		echo "$name: insmod ${TARGET_MODULE} unsuccessful, aborting now.."
		echo "dmesg|tail"
		dmesg|tail
		exit 5
	}
	echo "${name}: insmod ${TARGET_MODULE} successful."
	echo "dmesg|tail"
	dmesg|tail
fi
echo ${SEP}

# 2. Insert the helper_kp kernel module that will set up the kprobe
/sbin/insmod ./${KPMOD}.ko funcname=${FUNCTION} verbose=${VERBOSE} || {
	echo "${name}: insmod ${KPMOD} unsuccessful, aborting now.."
	if [ ${PROBE_KERNEL} -eq 0 ]; then
		/sbin/rmmod ${TARGET_MODULE}
	fi
	echo "dmesg|tail"
	dmesg|tail
	exit 7
}
echo "${name}: successful."
echo "dmesg|tail"
dmesg|tail
cd ..
}

usage()
{
	echo "Usage: ${name} [--verbose] [--help] [--mod=module-pathname] --probe=function-to-probe
       ---probe=probe-this-function  : if module-pathname is not passed, 
                                           then we assume the function to be kprobed is in the kernel itself.
       [--mod=module-pathname]       : pathname of kernel module that has the function-to-probe
       [--verbose]                   : run in verbose mode	
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
	zcat /proc/config.gz |grep -q -i kprobes && KPROBES_SUPPORTED=1
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

#TARGET_MODULE=""
SEP="-------------------------------------------------------------------------------"
name=$(basename $0)

if [ $(id -u) -ne 0 ]; then
	echo "${name}: requires root."
	exit 1
fi
kprobes_check

VERBOSE=0
optspec=":h?-:"  #mod:probe:"
while getopts "${optspec}" opt
do
    #echo "1. opt = ${opt}  ind=${OPTIND}  OPTARG = ${OPTARG}"
    case "${opt}" in
		-)                       # 'long' opts '--xxx' style, ala checksec!
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
			  *) echo "Unknown option '${OPTARG}'" #; usage
				;;
  	        esac
  	esac
done
shift $((OPTIND-1))

[ ${VERBOSE} -eq 1 ] && echo "FUNCTION=${FUNCTION} PROBE_KERNEL=${PROBE_KERNEL} TARGET_MODULE=${TARGET_MODULE} ; VERBOSE=${VERBOSE}"
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
		echo "${name}: kernel module '${TARGET_MODULE}' seems to be an invalid pathname, aborting now.."
		exit 1
	fi
	echo "Target kernel Module: ${TARGET_MODULE}"
fi

################ Generate a kernel module to probe this particular function ###############
BASEFILE_C=helper_kp.c
BASEFILE_H=convenient.h
BASEFILE=helper_kp

if [ ! -f ${BASEFILE_C} ]; then
  echo "${name}: base file ${BASEFILE_C} missing?"
  exit 1
fi
if [ ! -f ${BASEFILE_H} ]; then
  echo "${name}: base file ${BASEFILE_H} missing?"
  exit 1
fi

export KPMOD=${BASEFILE}-${FUNCTION}-$(date +%d%b%y) #_%H%M%S)
#export KPMOD=${BASEFILE}-${FUNCTION}-$(date +%d%m%y_%H%M%S)
echo $SEP
echo "KPMOD=${KPMOD}"

rm -rf tmp/ 2>/dev/null
mkdir -p tmp/
cd tmp
cp ../${BASEFILE_C} ${KPMOD}.c || exit 1
#cp ../${BASEFILE_H} . || exit 1

echo "--- Generating tmp/Makefile ---------------------------------------------------"
# Generate the Makefile
cat > Makefile << @MYMARKER@
# Makefile for kernel module
### Specifically: Kprobe helpers !

ifneq (\$(KERNELRELEASE),)
	$(info Dynamic Makefile:)
	$(info Building with KERNELRELEASE = ${KERNELRELEASE}) 
	# If you choose to keep the define USE_FTRACE_PRINT , we'll use
	# trace_printk() , else the regular printk()
	EXTRA_CFLAGS += -DDEBUG  # use regular printk()
	obj-m += ${KPMOD}.o

else
	#########################################
	# To support cross-compiling for the ARM:
	# For ARM, invoke make as:
	# make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- 
	ifeq (\$(ARCH),arm)
	# Update 'KDIR' below to point to the ARM Linux kernel source tree
		KDIR ?= ~/3.10.24
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

#rm -f convenient.h
ln -sf ../convenient.h  # adjust for your workspace

echo "--- make ---------------------------------------------------"
make || {
  echo "${name}: failed to 'make'. Aborting..."
  cd ..
  exit 1
}

ls -l ${KPMOD}.ko

##############################################

#echo "ARCH=$ARCH"
#TARGET_RFS_LOC=~/myprj   ## UPDATE! for your system
#if [ "${ARCH}" = "arm" ]; then
#	echo "Built module for ARM, copying to target RFS. Run on target to test.."
#	sudo cp ${KPMOD}.ko ${TARGET_RFS_LOC}
#	exit 0
#fi

insert_kprobe
exit 0

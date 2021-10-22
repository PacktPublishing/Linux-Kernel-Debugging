#!/bin/bash
# test_kmembugs.sh

# Also, IMP to boot system with 'kasan_multi_shot' :
# Pass kasan_multi_shot (add to kernel cmdline via /boot/cmdline.txt):
# From https://www.kernel.org/doc/html/latest/admin-guide/kernel-parameters.html
#  kasan_multi_shot
#  [KNL] Enforce KASAN (Kernel Address Sanitizer) to print
#  report on every invalid memory access. Without this
#  parameter KASAN will print report only for the first
#  invalid access.
name=$(basename $0)
KMOD=kmembugs_test

kasan_multi_shot_txt="  [KNL] Enforce KASAN (Kernel Address Sanitizer) to print
     report on every invalid memory access. Without this
     parameter KASAN will print report only for the first
     invalid access."
kconfig_chk=0

echo -n "$(uname -r): "
# this kernel built w/ CONFIG_KASAN on?
if [ -f /boot/config-$(uname -r) ] ; then
  grep -q "CONFIG_KASAN is not set" /boot/config-$(uname -r) && {
    echo "${name}: this kernel (ver $(uname -r)) doesn't seem to have CONFIG_KASAN configured... aborting"
    exit 1
  }
  if [ -f /boot/config-$(uname -r) ] ; then
	 grep -q "CONFIG_KASAN=y" /boot/config-$(uname -r) && kconfig_chk=1
  fi
elif [ -f /proc/config.gz ] ; then
  zcat /proc/config.gz | grep -q "CONFIG_KASAN is not set" && {
    echo "${name}: this kernel (ver $(uname -r)) doesn't seem to have CONFIG_KASAN configured... aborting.
  Please configure this or a custom kernel with CONFIG_KASAN and retry ..."
    exit 1
  }
  if [ -f /proc/config.gz ] ; then
	 grep -q "CONFIG_KASAN=y" /boot/config-$(uname -r) && kconfig_chk=1
  fi
	  #grep -q "CONFIG_KASAN=y" /boot/config-$(uname -r) && kconfig_chk=1
       	  #kconfig_chk=1
	  #echo "CONFIG_KASAN is set"
else
  sudo modprobe configs
  zcat /proc/config.gz | grep -q "CONFIG_KASAN is not set" && {
    echo "${name}: this kernel (ver $(uname -r)) doesn't seem to have CONFIG_KASAN configured... aborting.
  Please configure this or a custom kernel with CONFIG_KASAN and retry ..."
    exit 1
  }
  if [ -f /proc/config.gz ] ; then
	 grep -q "CONFIG_KASAN=y" /boot/config-$(uname -r) && kconfig_chk=1
  fi
	#  grep -q "CONFIG_KASAN=y" /boot/config-$(uname -r) && kconfig_chk=1
       	  #kconfig_chk=1
	  #echo "CONFIG_KASAN is set"
fi
[ ${kconfig_chk} -eq 0 ] && {
    echo "${name}: this kernel (ver $(uname -r)) doesn't seem to have CONFIG_KASAN configured... aborting.
  Please configure this or a custom kernel with CONFIG_KASAN and retry ..."
  exit 1
} || echo "CONFIG_KASAN is set"

[ ! -f ${KMOD}.ko ] && {
  echo "${name}: kernel module \"${KMOD}.ko\" not present? Aborting ...
  Build it first and retry...
  Also, did you remember to boot the system with the kernel cmdline parameter
  'kasan_multi_shot':
  ${kasan_multi_shot_txt}"
  exit 1
}
ls -l ${KMOD}.ko

[ $# -ne 1 ] && {
 echo "Usage: ${name} testcase#"
 echo "
        test case  1 : uninitialized var (UMR) test case
	test case  2 : out-of-bounds : write overflow [on compile-time memory]
	test case  3 : out-of-bounds : write overflow [on dynamic memory]
	test case  4 : out-of-bounds : write underflow
	test case  5 : out-of-bounds : read overflow [on compile-time memory]
	test case  6 : out-of-bounds : read overflow [on dynamic memory]
	test case  7 : out-of-bounds : read underflow
	test case  8 : UAF (use-after-free)
	test case  9 : UAR (use-after-return)
	test case 10 : double-free
	test case 11 : memory leak : simple leak

  Also, did you remember to boot the system with the kernel cmdline parameter
  'kasan_multi_shot':
   ${kasan_multi_shot_txt}"
# exit 1
}

[ -f /proc/cmdline ] && grep -q "kasan_multi_shot" /proc/cmdline
if [ $? -ne 0 ] ; then
   echo "WARNING!
It appears you have NOT passed the kernel cmdline parameter 'kasan_multi_shot'
at boot. Here's the explanation:
   ${kasan_multi_shot_txt}
We recommend you pass it when testing this kernel module.
"
else
   echo "Kernel parameter kasan_multi_shot passed, KASAN won't stop at first issue"
fi

echo "sudo rmmod ${KMOD} 2>/dev/null ; sudo dmesg -C; sudo insmod ./${KMOD}.ko testcase=${1}; dmesg"
sudo rmmod ${KMOD} 2>/dev/null ; sudo dmesg -C; sudo insmod ./${KMOD}.ko testcase=${1}; dmesg

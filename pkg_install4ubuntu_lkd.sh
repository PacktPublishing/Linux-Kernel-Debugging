#!/bin/bash
# pkg_install4ubuntu_lkd.sh
# ***************************************************************
# This program is part of the source code released for the book
#  "Linux Kernel Debugging"
#  (c) Author: Kaiwan N Billimoria
#  Publisher:  Packt
#  GitHub repository:
#  https://github.com/PacktPublishing/Linux-Kernel-Debugging
#***************************************************************
# Brief Description:
# Helper script to install all required packages (as well as a few more
# possibly) for the Linux Kernel Debugging book.

function die
{
	echo >&2 "$@"
	exit 1
}
runcmd()
{
    [ $# -eq 0 ] && return
    echo "$@"
    eval "$@" || die "failed"
}

runcmd sudo apt update

# packages typically required for kernel build
runcmd sudo apt install -y bison flex libncurses5-dev ncurses-dev xz-utils libssl-dev libelf-dev util-linux tar

# other packages...
runcmd sudo apt install -y bc bpfcc-tools bsdmainutils clang cmake cppcheck cscope curl \
 dwarves exuberant-ctags fakeroot flawfinder git gnome-system-monitor gnuplot \
 hwloc indent kernelshark libnuma-dev libjson-c-dev linux-tools-$(uname -r) \
 net-tools numactl openjdk-16-jre openssh-server perf-tools-unstable psmisc \
 python3-distutils rt-tests smem sparse stress sysfsutils tldr-py trace-cmd \
 tree tuna virt-what

# Add yourself to the vboxsf group (to gain access to VirtualBox shared folders);
# will require you to log out and back in (or even reboot) to take effect
groups |grep -q -w vboxsf || runcmd sudo usermod -G vboxsf -a ${USER}

exit 0

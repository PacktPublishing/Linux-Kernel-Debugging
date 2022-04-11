#!/bin/sh
# gdbline module image
#
# Outputs an add-symbol-file line suitable for pasting into gdb to examine
# a loaded module.
# (Here used in conjunction with the hello_gdb.ko kernel module; gdb demo).
#
# Credit: mostly based on the script from the venerable LDD3 book!
name=$(basename $0)
[ $(id -u) -ne 0 ] && {
	echo "${name} requires root."
	exit 1
}

if [ $# -ne 2 ]; then
	echo "Usage: ${name} module-name image-filename"
	echo "  module-name: name of the (already inserted) kernel module (without the .ko)"
	echo "  image-filename: pathname to the kernel module."
	exit 1
fi
if [ ! -d /sys/module/$1/sections ]; then
	echo "${name}: $1: module not inserted?"
	exit 1
fi
if [ ! -f $2 ]; then
	echo "${name}: $2 not a valid file"
	exit 1
fi

cd /sys/module/$1/sections
echo "Copy-paste the following lines into GDB"
echo "---snip---"

[ -f .text ] && {
   echo -n add-symbol-file $2 $(/bin/cat .text)
   echo  " \\"
} || [ -f .init.text ] && {
   echo -n add-symbol-file $2 $(/bin/cat .init.text)
}

for section in .[a-z]* *; do
    if [ ${section} != ".text" -o ${section} != ".init.text" ]; then
	    echo  " \\"
	    echo -n "       -s" ${section} $(/bin/cat ${section})
    fi
done
echo "
---snip---"
echo

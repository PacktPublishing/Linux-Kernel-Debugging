README.txt
Ch 11 - Using KGDB

(As explained in Ch 11 - Using Kernel GDB (KGDB) section Technical requirements):

For the section 'Debugging kernel modules with KGDB':
These are the source files:

ch11 $ tree .
.
├── gdbline.sh
├── images
│   └── rootfs_deb.img
├── kconfig_x86-64_target
├── kgdb_try
│   ├── kgdb_try.c
│   └── Makefile
├── README.txt
├── rootfs_deb.img.7z
└── run_target.sh

2 directories, 8 files
ch11 $ 

In addition, you'll require the compressed target root filesystem image.
Download it:
wget https://github.com/PacktPublishing/Linux-Kernel-Debugging/raw/main/ch11/rootfs_deb.img.7z

You'll get the 7zip file rootfs_deb.img.7z; extract it:
7z x rootfs_deb.img.7z

You'll get the uncompressed rootfs image file images/rootfs_deb.img (size 512 MB).
Use it (and the other source files) as explained in the book, Ch 11 section
'Debugging kernel modules with KGDB'.

README.txt
Ch 11 - Using KGDB

(As explained in Ch 11 - 'Using Kernel GDB (KGDB)' section 'Technical requirements'):

For the section 'Debugging kernel modules with KGDB':
These are the source files to begin with; this is after you've downloaded the 7zip file...
The explanation of how to download and set it up is in the 'Technical requirements'
section of this chapter:

ch11 $ tree .
.
├── gdbline.sh
├── kconfig_x86-64_target
├── kgdb_try
│   ├── kgdb_try.c
│   └── Makefile
├── README.txt
├── rootfs_deb.img.7z
└── run_target.sh

1 directory, 7 files


Now extract it the root filesystem:
ch11 $ 7z x rootfs_deb.img.7z
[...]

You'll get the uncompressed rootfs image file images/rootfs_deb.img (size 512 MB).

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

Use it (and the other source files) as explained in the book, Ch 11 section
'Debugging kernel modules with KGDB'.

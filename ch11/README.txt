README.txt
Ch 11 - Using KGDB

For the section 'Debugging kernel modules with KGDB':
These are the source files:

ch11 $ tree .
.
├── gdbline.sh
├── kgdb_try
│   ├── kgdb_try.c
│   └── Makefile
├── README.txt
└── run_target.sh

1 directory, 5 files
ch11 $ 

In addition, you'll require the target root filesystem image; it's downloadable
here:
https://drive.google.com/drive/folders/1YGYkPCGMiRaI__x6-r1eoIVgEzzr_e7P?usp=sharing

Download it; you'll get the 7zip file rootfs_deb.img.7z; extract it:
7z x rootfs_deb.img.7z

You'll get the uncompressed rootfs image file rootfs_deb.img (size 512MB).
Use it (and the other source files) as explained in the book, Ch 11 section
'Debugging kernel modules with KGDB'.

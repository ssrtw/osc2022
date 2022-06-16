#!/bin/sh
cd rootfs
./build_proc.sh
find . | cpio -o -H newc > ../initramfs.cpio
cd ..
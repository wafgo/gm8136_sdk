genext2fs -b 10240 -i 1024 -d rootfs-cpio ramdisk
lzma -kvf ramdisk
cp ramdisk.lzma /tftpboot/

#gzip -9 -f ramdisk
#cp ramdisk.gz /tftpboot/

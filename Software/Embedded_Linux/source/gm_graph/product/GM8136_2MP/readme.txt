Collect sample, param.cfg, spec.cfg, vg_boot.sh in "image" directory.
And use:

# mkfs.jffs2 -e 0x20000 -n -d image -o GM8136_2MP.NAND.jffs2.img
# mkfs.jffs2 -e 0x10000 -n -d image -o GM8136_2MP.SPI.jffs2.img

to build product image (jffs2).

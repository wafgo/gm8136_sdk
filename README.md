# gm8136_sdk
source code for the system running on the grain media gm8136 soc which is used by many cheap webcams

Everything in this repo is prehistoric:

- GCC v4.4.0
- Buildroot 2012.02
- U-Boot 2013.01
- Linux Kernel 3.3

# Reverse Engineering

## Bootrom
The bootrom of the SoC is completely undocumented (at least i was not able to find anything about its operation online). What i could reverse engineer so far is that the bootrom loads the first stage loader from the NVM (SPI NOR/NAND) from address **0x1000** to the (undocumented) SRAM Address at **0x92000000** (This memory areas is only marked as reserved, but no mention that it is SRAM in the memory map). 

The first stage loader is called (for an unknown reason **nsboot**). The source code of nsboot is located under Software/Embedded_Linux/source/gm8136_nsboot. You can compile it with the cross gcc e.g. from linaro (for example this here https://releases.linaro.org/components/toolchain/binaries/7.5-2019.12/arm-linux-gnueabi/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabi.tar.xz).

**ATTENTION: Do not use the arm-linux-gnueabihf because this one can only be used with VFP (hardware fpu -> which should be the reason why the HF is appended to the toolcain triplet) which the gm8136 core arm926ejs (armv4 ISA) doesn´t have (soft-fpu only). Any effort doing so will result in a not linkable libgcc, which seems to have VFP support hardcoded into the library** 

### How the Bootrom functionality was reverse engineered
This analysis was done by reading out the XM25QH128A located on the ESOLOM wlan webcam. The reading can be done by two different methods

1: Connect the UART wires to your PC. Let the system boot and press any key to stop in u-boot
  - insert an mmc card into the slot (ATTENTION: with this method you will destroy the MBR/GPT of the mmc card, only use it if it does not contain important data)
  - type in into the uart prompt: **sf read 0x2000000 0 0x1000000; mmc write 0x2000000 0 0x8000**. This will write the full SPI flash (16MB) content at the beginning of the mmc 
  - insert the mmc card into your machine
  - run: **dd if=/dev/$DEV_OF_YOUR_EMMC of=~/fw.bin bs=512 count=32768**
  
  
2: Buy some cheap BIOS programmer like this here: https://www.amazon.de/KeeYees-Programmer-Konverter-Motherboard-Routing/dp/B07SHSRFS8 and hook it up to the SPI chip on the board
  - install the flashrom utility (e.g. build it from here https://github.com/flashrom/flashrom)
  - run: **flashrom --programmer ch341a_spi -r ~/fw.bin**
  
 In both cases you will get the **~/fw.bin** binary containing the full SPI flash content which can be analysed (e.g. using the awesome binwalk utility from here: https://github.com/ReFirmLabs/binwalk.
 
 #### Building nsboot
 
 To find out from which location the bootrom loads the nsboot, nsboot was build from source using the linaro toolchain (link see above). Simply open the Makefile and replace the CROSS_COMPILE variable to point to the downloaded linaro toolchain. 
 
Now run **make**

You will get the nsboot elf and **nsboot.bin** (which is the interesting one)

Below you see the **nsboot.bin** side by side to **fw.bin**
[find_nsboot_in_fw]: https://github.com/wafgo/gm8136_sdk/images/nsboot_contained_in_fw.png "nsboot.bin is located at 0x1000 on the spi flash"

We can see that it is located at a offset of **0x1000** in the SPI flash.

After it was clear that nsboot was loaded from 0x1000 on the serial flash, the question was where it is loaded at. This is not hard to find out having the elf.
  
Executing **readelf -S nsboot** gives us that the first loadable section (.text) is at **0x92000000**. As this is the linked address this rises the suspision that this is the address where the BOOTROM will load nsboot to (or any other first stage loader). The linker script (or executing **addr2line -e nsboot 0x92000000**) shows that the first instruction in the text section is at head.S and is the **_start** symbol. 

Currently i was not able to find any entry point (or offset) so I assume that the Bootrom will jump directly to the loaded start address.

## nsboot

The first stage loader is responsible to initialize the DRAM and load u-boot into this memory. For this the SPI flash conains a header starting at address 0x0 (MBR). This has the following layout:

```c
struct nand_head
{
    char signature[8];          /* Signature is "GM8xxx" */
    uint32_t bootm_addr;    /* Image offset to load by spiboot */
    uint32_t bootm_size;
    uint32_t bootm_reserved;
    struct {
        uint32_t addr;          /* image address */
        uint32_t size;          /* image size */
        unsigned char name[8];      /* image name */
        uint32_t reserved[1];
    } image[10];
    struct {
        uint32_t nand_numblks;      //number of blocks in chip
        uint32_t nand_numpgs_blk;   //how many pages in a block
        uint32_t nand_pagesz;       //real size in bytes
        uint32_t nand_sparesz_inpage;       //64bytes for 2k, ...... needed for NANDC023
        uint32_t nand_numce;        //how many CE in chip
        uint32_t nand_clkdiv;				// 2/4/6/8
        uint32_t nand_ecc_capability;
        uint32_t nand_sparesz_insect;
    } nandfixup;
    uint32_t reserved2[64];        // unused
    unsigned char last_511[4];     //MBR magic
} __attribute__((packed));

```

  

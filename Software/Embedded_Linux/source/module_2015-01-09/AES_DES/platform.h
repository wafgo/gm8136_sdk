#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#define MODULE_NAME             "security"

/* register */
#define SEC_EncryptControl      0x00000000
#define SEC_SlaveRunEnable      0x00000004
#define SEC_FIFOStatus          0x00000008
#define SEC_PErrStatus          0x0000000C
#define SEC_DESKey1H            0x00000010
#define SEC_DESKey1L            0x00000014
#define SEC_DESKey2H            0x00000018
#define SEC_DESKey2L            0x0000001C
#define SEC_DESKey3H            0x00000020
#define SEC_DESKey3L            0x00000024
#define SEC_AESKey6             0x00000028
#define SEC_AESKey7             0x0000002C
#define SEC_DESIVH              0x00000030
#define SEC_DESIVL              0x00000034
#define SEC_AESIV2              0x00000038
#define SEC_AESIV3              0x0000003C
#define SEC_INFIFOPort          0x00000040
#define SEC_OutFIFOPort         0x00000044
#define SEC_DMASrc              0x00000048
#define SEC_DMADes              0x0000004C
#define SEC_DMATrasSize         0x00000050
#define SEC_DMACtrl             0x00000054
#define SEC_FIFOThreshold       0x00000058
#define SEC_IntrEnable          0x0000005C
#define SEC_IntrStatus          0x00000060
#define SEC_MaskedIntrStatus    0x00000064
#define SEC_ClearIntrStatus     0x00000068
#define SEC_LAST_IV0            0x00000080
#define SEC_LAST_IV1            0x00000084
#define SEC_LAST_IV2            0x00000088
#define SEC_LAST_IV3            0x0000008c

#if defined(CONFIG_PLATFORM_GM8129) || defined(CONFIG_PLATFORM_GM8287) || defined(CONFIG_PLATFORM_GM8136)
#define MAX_SEC_DMATrasSize ((((4<<30)-1)>>2)<<2)   ///< 4 Alignment
#else
#define MAX_SEC_DMATrasSize ((((4<<10)-1)>>2)<<2)   ///< 4 Alignment
#endif

/* bit mapping of command register */
//SEC_EncryptControl
#define Parity_check        0x100
#define First_block         0x80

//SEC_DMACtrl
#define DMA_Enable          0x1

//SEC_IntrStatus
#define Data_done           0x1

#define Decrypt_Stage       0x1
#define Encrypt_Stage       0x0

/*
 * Function Declarations
 */
int platform_pmu_init(void);
int platform_pmu_exit(void);

#endif /* __PLATFORM_H__ */

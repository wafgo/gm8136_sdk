#ifndef __CPU_COMM_H__
#define __CPU_COMM_H__

#define CPU_COMM_VERSION    "2.0.0"
//#define PFX		            "CPU_COMM"

#ifdef __KERNEL__

/* Transaction CPU to CPU */
#define TRANS_CPU_726_626       0x1 /* both direction */
#define TRANS_CPU_626_7500      0x2
#define TRANS_CPU_726_7500      0x3
#define TRANS_CPU_726_726       0x4

/* 5 PCIs */
#define TRANS_PCI0  0x1
#define TRANS_PCI1  0x2
#define TRANS_PCI2  0x3
#define TRANS_PCI3  0x4
#define TRANS_PCI4  0x5

#define TRANACTION_ID(pci, cpu, ch)     (((pci) << 12) | ((cpu) << 8) | (ch))

/* x is transaction id */
#define CHAN_ID(x)  ((x) & 0xFF)
#define BOARD_ID(x) (((x) >> 8) << 8)  //include pci part
#define PCIX_ID(x)  ((((x) >> 12) & 0xF) << 12)
#define IS_PCI(x)   ((x)->board_id >> 12)

/* define the channel base in each CPU */
#define CHAN_726_626_BASE       TRANACTION_ID(0, TRANS_CPU_726_626, 0)  //FA726 <->FA626
#define CHAN_626_7500_BASE      TRANACTION_ID(0, TRANS_CPU_626_7500, 0) //FA626 <-> FC7500
#define CHAN_726_7500_BASE      TRANACTION_ID(0, TRANS_CPU_726_7500, 0) //FA726 <-> FC7500
#define CHAN_726_726_BASE       TRANACTION_ID(0, TRANS_CPU_726_726, 0)  //FA726 <-> FA726

/* pci-0 */
#define CHAN_PCI0_726_726_BASE  TRANACTION_ID(TRANS_PCI0, TRANS_CPU_726_726, 0)
#define CHAN_PCI0_726_626_BASE  TRANACTION_ID(TRANS_PCI0, TRANS_CPU_726_626, 0)
/* pci-1 */
#define CHAN_PCI1_726_726_BASE  TRANACTION_ID(TRANS_PCI1, TRANS_CPU_726_726, 0)
#define CHAN_PCI1_726_626_BASE  TRANACTION_ID(TRANS_PCI1, TRANS_CPU_726_626, 0)


/* the transaction id consists of
 * bit 0-7: channel
 * bit 8-11: board id(cpu id)
 * bit 12-15: pcix id
 */
typedef enum {
    /* FA726 <-> FA626 */
    CHAN_0_USR0 = CHAN_726_626_BASE,    /* DVR */
    CHAN_0_USR1,    /* DVR */
    CHAN_0_USR2,    /* DVR */
    CHAN_0_USR3,    /* DVR */
    CHAN_0_USR4,    /* HDMI */
    CHAN_0_USR5,    /* Front end */
    CHAN_0_USR6,    /* LCD0 */
    CHAN_0_USR7,    /* LCD2 */
    CHAN_0_USR8,    /* SCALER */
    CHAN_0_USR9,    /* UNUSED */
    CHAN_0_USR10,   /* UNUSED */
    CHAN_0_USR11,   /* UNUSED */
    CHAN_0_USR12,   /* AP */
    CHAN_0_USR13,   /* AP */
    CHAN_0_USR14,   /* AP */
    CHAN_0_MAX,

    /* FA626 <-> FC7500 */
    CHAN_1_USR0 = CHAN_626_7500_BASE,
    CHAN_1_USR1,
    CHAN_1_USR2,
    CHAN_1_USR3,
    CHAN_1_USR4,
    CHAN_1_USR5,
    CHAN_1_USR6,
    CHAN_1_USR7,
    CHAN_1_USR8,
    CHAN_1_USR9,
    CHAN_1_USR10,
    CHAN_1_USR11,
    CHAN_1_USR12,
    CHAN_1_USR13,
    CHAN_1_USR14,
    CHAN_1_MAX,

    /* FA726 <-> FC7500 */
    CHAN_2_USR0 = CHAN_726_7500_BASE,
    CHAN_2_USR1,
    CHAN_2_USR2,
    CHAN_2_USR3,
    CHAN_2_USR4,
    CHAN_2_USR5,
    CHAN_2_USR6,
    CHAN_2_USR7,
    CHAN_2_USR8,
    CHAN_2_USR9,
    CHAN_2_USR10,
    CHAN_2_USR11,
    CHAN_2_USR12,
    CHAN_2_USR13,
    CHAN_2_USR14,
    CHAN_2_MAX,

    /* PCI0, FA726 <-> FA726 */
    CHAN_PCI0_0_USR0 = CHAN_PCI0_726_726_BASE,
    CHAN_PCI0_0_USR1,
    CHAN_PCI0_0_USR2,
    CHAN_PCI0_0_USR3,
    CHAN_PCI0_0_USR4,
    CHAN_PCI0_0_USR5,
    CHAN_PCI0_0_USR6,
    CHAN_PCI0_0_USR7,
    CHAN_PCI0_0_USR8,
    CHAN_PCI0_0_USR9,
    CHAN_PCI0_0_USR10,
    CHAN_PCI0_0_USR11,
    CHAN_PCI0_0_USR12,  /* AP */
    CHAN_PCI0_0_USR13,  /* AP */
    CHAN_PCI0_0_USR14,  /* AP */
    CHAN_PCI0_0_MAX,

    /* PCI0, FA726 <-> FA626 */
    CHAN_PCI0_1_USR0 = CHAN_PCI0_726_626_BASE,
    CHAN_PCI0_1_USR1,
    CHAN_PCI0_1_USR2,
    CHAN_PCI0_1_USR3,
    CHAN_PCI0_1_USR4,
    CHAN_PCI0_1_USR5,
    CHAN_PCI0_1_USR6,
    CHAN_PCI0_1_USR7,
    CHAN_PCI0_1_USR8,
    CHAN_PCI0_1_USR9,
    CHAN_PCI0_1_USR10,
    CHAN_PCI0_1_USR11,
    CHAN_PCI0_1_USR12,  /* AP */
    CHAN_PCI0_1_USR13,  /* AP */
    CHAN_PCI0_1_USR14,  /* AP */
    CHAN_PCI0_1_MAX,

    /* not define yet */
    CHAN_PCI1_0_USR0 = CHAN_PCI1_726_726_BASE,
    CHAN_PCI1_0_MAX,
    CHAN_PCI1_1_USR0 = CHAN_PCI1_726_626_BASE,
    CHAN_PCI1_1_MAX,
} chan_id_t;

typedef struct {
    chan_id_t       target;
    int             length; /* msg length */
    unsigned char   *msg_data;
} cpu_comm_msg_t;

#define CPU_COMM_NUM_IN_PACK    36

typedef struct {
	chan_id_t		target;
	int				length[CPU_COMM_NUM_IN_PACK];	/* length 0 means the last data */
	unsigned char	*msg_data[CPU_COMM_NUM_IN_PACK];
} cpu_comm_pack_msg_t;

/*
 * @This function registers a client to CPU_COMM module. When the frames come, it will be delivered
 *      to the channel of this reigstered client.
 *
 * @function int cpu_comm_open(chan_id_t ch_id, char *name);
 * @param chan_id_t indicates the channel id prdefined in chan_id_t
 * @param name indicates your name.
 * @return 0 on success, <0 on error
 */
int cpu_comm_open(chan_id_t chan_id, char *name);

/*
 * @This function de-registers a client from CPU_COMM module. When this function is called,
 *  all frames queued in this channel are removed.
 *
 * @function int cpu_comm_close(chan_id_t ch_id, char *name);
 * @param chan_id_t indicates the channel id prdefined in chan_id_t
 * @return 0 on success, <0 on error
 */
int cpu_comm_close(chan_id_t chan_id);

/*
 * @This function sends the msg into the channel.
 *
 * @function int cpu_comm_msg_snd(cpu_comm_msg_t *msg, int timeout_ms);
 * @param msg indicates the data the module wants to send. Note: msg->channel is
 *      necessary. It indicates the destination.
 * @param timeout_ms indicates whether the caller will be blocked if the ring buffer is full or the
 *      buffer of the peer is full.
 *      timeout_ms: 0 for not wait, -1 for wait forever, >= 1 means a timeout value
 * @return 0 on success, <0 on error
 */
int cpu_comm_msg_snd(cpu_comm_msg_t *msg, int timeout_ms);

/*
 * @This function receives the msg from the channel.
 *
 * @function int cpu_comm_msg_rcv(cpu_comm_msg_t *msg, int timeout_ms);
 * @param msg indicates the data the module wants to receive. Note: msg->channel is
 *      necessary. It indicates the destination.
 * @param timeout_ms indicates whether the caller will be blocked if the channel is empty.
 *      timeout_ms: 0 for not wait, -1 for wait forever, >= 1 means a timeout value
 * @return 0 on success, <0 on error
 */
int cpu_comm_msg_rcv(cpu_comm_msg_t *msg, int timeout_ms);

/*
 * @This function registers a client to CPU_COMM module. When the frames come, it will be delivered
 *      to the channel of this reigstered client.
 *
 * @function int cpu_comm_pack_open(chan_id_t ch_id, char *name);
 * @param chan_id_t indicates the channel id prdefined in chan_id_t
 * @param name indicates your name.
 * @return 0 on success, <0 on error
 * @Note: pack open and close works in pair
 */
int cpu_comm_pack_open(chan_id_t chan_id, char *name);

/*
 * @This function de-registers a client from CPU_COMM module. When this function is called,
 *  all frames queued in this channel are removed.
 *
 * @function int cpu_comm_pack_close(chan_id_t ch_id, char *name);
 * @param chan_id_t indicates the channel id prdefined in chan_id_t
 * @return 0 on success, <0 on error
 * @Note: pack open and close works in pair
 */
int cpu_comm_pack_close(chan_id_t chan_id);

/*
 * @This function sends the packed msg into the channel.
 *
 * @function int cpu_comm_pack_msg_snd(cpu_comm_pack_msg_t *msg, int timeout_ms);
 * @param msg indicates the data the module wants to send. Every parameter is necessary.
 * @param timeout_ms indicates whether the caller will be blocked if the ring buffer is full or the
 *      buffer of the peer is full.
 *      timeout_ms: 0 for not wait, -1 for wait forever, >= 1 means a timeout value
 * @return 0 on success, <0 on error
 * @Note: pack send and recv works in pair
 */
int cpu_comm_pack_msg_snd(cpu_comm_pack_msg_t *msg, int timeout_ms);

/*
 * @This function receives the packed msg from the channel.
 *
 * @function int cpu_comm_pack_msg_rcv(cpu_comm_pack_msg_t *msg, int timeout_ms);
 * @param msg indicates the data the module wants to receive.
 * @param timeout_ms indicates whether the caller will be blocked if the channel is empty.
 *      timeout_ms: 0 for not wait, -1 for wait forever, >= 1 means a timeout value
 * @return 0 on success, <0 on error
 * @Note: pack send and recv works in pair
 */
int cpu_comm_pack_msg_rcv(cpu_comm_pack_msg_t *msg, int timeout_ms);

/*
 * @This function updates the buffer list length for queuing the data from upper layer.
 *
 * @function int cpu_comm_config_rxtx_qlen(chan_id_t chan_id, int length);
 * @param: chan_id indicates the specific channel
 *         length: queue length
 * @return 0 for success, others for fail.
 */
int cpu_comm_config_rxtx_qlen(chan_id_t chan_id, int length);

/*
 * @This function get the max buffer size without truncating message.
 *
 * @function int cpu_comm_get_msgsz(chan_id_t chan_id);
 * @param: chan_id indicates the specific channel
 *
 * @return the max message size if > 0, <= 0 for fail.
 */
int cpu_comm_get_msgsz(chan_id_t chan_id);

/* This function is used to reset the on-hand entry if the user doesn't want to process this
 * entry anymore.
 * Note: the packets queuing in the readlist are not affected.
 */
void cpu_comm_pack_clean_readentry(chan_id_t chan_id);

typedef enum {
    PCIX_HOST = 0,
    PCIX_DEV0,
    PCIX_OFF,   /* not PCI device exist */
    PCIX_MAX = PCIX_OFF,
} pcix_id_t;

typedef enum {
    CPU_FA726 = 0,
    CPU_FA626,
    CPU_FC7500,
    CPU_MAX,
} cpu_id_t;
/*
 * @This function returns who I am.
 *
 * @function int cpu_comm_get_identifier(int *chip_id, int *subsys_id)
 * @param: pcix_id indicates the pcix id in the whole system
 * @param: cpu_id_t indicates the numbered cpu id within one chip
 * @return 0 for success others for fail.
 */
int cpu_comm_get_identifier(pcix_id_t *pcix_id, cpu_id_t *cpu_id);

/*
 * @This function returns the ticks
 *
 * @function int cpu_comm_get_tick(pcix_id_t pci_id, cpu_id_t cpu_id, u32 *tick_in_hz, u32 *tick)
 * @param: pci_id indicates the pcix id in the whole system. Current it will be ingored...
 * @param: cpu_id_t indicates the numbered cpu id within one chip
 * @param: tick_in_hz indicates number of ticks in a second (hz)
 * @param: tick indicates current jiffies
 * @return 0 for success others for fail.
 */
int cpu_comm_get_tick(pcix_id_t pci_id, cpu_id_t cpu_id, u32 *tick_in_hz, u32 *tick);

#endif /* __KERNEL__ */

// ioctl
#define CPU_COMM_MAGIC 'h'

#define CPU_COMM_READ_MAX       _IOR(CPU_COMM_MAGIC, 1, unsigned int)
#define CPU_COMM_WRITE_MAX      _IOR(CPU_COMM_MAGIC, 2, unsigned int)
#define CPU_COMM_MEM_ATTR       _IOW(CPU_COMM_MAGIC, 3, unsigned int)   /* MEM_ATTR_FRAMMAP_CACHE... */
    #define MEM_ATTR_FRAMMAP_CACHE     0x1  /* memory is from frammap which is cachable */
    #define MEM_ATTR_FRAMMAP_NONCACHE  0x2  /* memory is from frammap which is non-cachable */

/* For backward compatible */
#define CHAN_PCIX_0_USR0    CHAN_PCI0_1_USR0
#define CHAN_PCIX_0_MAX     CHAN_PCI0_1_MAX

#endif /* __CPU_COMM_H__ */


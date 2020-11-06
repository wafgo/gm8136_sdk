#ifndef _FE_PLAT_CPUCOMM_H_
#define _FE_PLAT_CPUCOMM_H_

struct plat_cpucomm_msg {
    int           length;
    unsigned char *msg_data;
};

#ifdef CONFIG_PLATFORM_GM8210
#include "cpu_comm/cpu_comm.h"

#define PLAT_CPUCOMM_CHAN       CHAN_0_USR5

static inline int plat_cpucomm_open(void)
{
    return cpu_comm_open(PLAT_CPUCOMM_CHAN, "FE_COMM");
}

static inline int plat_cpucomm_close(void)
{
    return cpu_comm_close(PLAT_CPUCOMM_CHAN);
}

static inline int plat_cpucomm_tx(struct plat_cpucomm_msg *msg, int timeout)
{
    cpu_comm_msg_t tx_msg;

    tx_msg.target   = PLAT_CPUCOMM_CHAN;
    tx_msg.length   = msg->length;
    tx_msg.msg_data = msg->msg_data;

    return cpu_comm_msg_snd(&tx_msg, timeout);
}

static inline int plat_cpucomm_rx(struct plat_cpucomm_msg *msg, int timeout)
{
    int ret;
    cpu_comm_msg_t rx_msg;

    rx_msg.target   = PLAT_CPUCOMM_CHAN;
    rx_msg.length   = msg->length;
    rx_msg.msg_data = msg->msg_data;

    ret = cpu_comm_msg_rcv(&rx_msg, timeout);
    if(ret < 0) {
        msg->length = 0;
        goto exit;
    }

    msg->length = rx_msg.length;    ///< update real received data length

exit:
    return ret;
}
#else
static inline int plat_cpucomm_open(void)
{
    return -1;
}

static inline int plat_cpucomm_close(void)
{
    return -1;
}

static inline int plat_cpucomm_tx(struct plat_cpucomm_msg *msg, int timeout)
{
    return -1;
}

static inline int plat_cpucomm_rx(struct plat_cpucomm_msg *msg, int timeout)
{
    return -1;
}
#endif

#endif  /* _FE_PLAT_CPUCOMM_H_ */

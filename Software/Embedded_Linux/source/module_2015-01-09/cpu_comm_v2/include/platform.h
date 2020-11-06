#ifndef _PLATFORM_H_
#define _PLATFORM_H_

int platform_doorbell_init(zebra_board_t *brd);
void platform_doorbell_exit(zebra_board_t *brd);
void platform_trigger_interrupt(zebra_board_t *, unsigned int);
pcix_id_t platform_get_pci_type(void);
void platform_set_inbound_win(zebra_board_t *brd, u32 axi_base);
void platform_set_outbound_win(zebra_board_t *brd, u32 pci_base);
void platform_pci_init(zebra_board_t *brd);
void platform_pci_exit(zebra_board_t *brd);
u32 platform_get_dma_chanmask(void);

#endif /* _PLATFORM_H_ */
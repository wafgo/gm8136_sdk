#ifndef SCALER_DIRECTIO_H
#define SCALER_DIRECTIO_H

#define DIO_VERSION     0x20140822
#define ENTRY_COUNT     64

#define DIO_BUFIDX(x)	            ((x) & (ENTRY_COUNT - 1))
#define DIO_BUFIDX_DIST(head, tail)	((long)(head) - (long)(tail))
#define DIO_BUF_SPACE(head, tail)	(ENTRY_COUNT - DIO_BUFIDX_DIST(head, tail))
#define DIO_BUF_COUNT(head, tail)	DIO_BUFIDX_DIST(head, tail)

typedef struct {
    unsigned int    data[10];
} directio_info_t;

void scaler_trans_init(void);
void scaler_trans_exit(void);
int scaler_directio_snd(directio_info_t *data, int len);
int scaler_directio_rcv(directio_info_t *data, int len);

#endif /* SCALER_DIRECTIO_H */
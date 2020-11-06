#ifndef __IRDA_API_H__
#define __IRDA_API_H__


typedef struct irda_data_tag
{
    int val;
    int repeat;

}irda_pub_data;

#define IRDET_IOC_MAGIC   'I'
#define IRDET_SET_DELAY_INTERVAL             _IOW(IRDET_IOC_MAGIC, 0, int)

#endif /*__IRDA_API_H__*/


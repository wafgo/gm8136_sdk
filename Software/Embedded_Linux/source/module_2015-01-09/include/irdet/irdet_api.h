#ifndef __IRDET_API_H__
#define __IRDET_API_H__


typedef struct irdet_data_tag
{
    int val;
    int repeat;

}irdet_data;

#define IRDET_IOC_MAGIC   'I'
#define IRDET_SET_DELAY_INTERVAL             _IOW(IRDET_IOC_MAGIC, 0, int)

#endif /*__IRDET_API_H__*/


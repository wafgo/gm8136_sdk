#ifndef __KEYSCAN_API_H__
#define __KEYSCAN_API_H__

//key status
#define KEY_IN      1
#define KEY_REPEAT  2

typedef struct keyscan_data_tag
{
    int val;
    int status;

}keyscan_data;

#define KEYSCAN_IOC_MAGIC   'I'
#define KEYSCAN_SET_SCAN_INTERVAL             _IOW(KEYSCAN_IOC_MAGIC, 0, int)
#define KEYSCAN_SET_REPEAT_INTERVAL           _IOW(KEYSCAN_IOC_MAGIC, 1, int)

#endif /*__KEYSCAN_API_H__*/


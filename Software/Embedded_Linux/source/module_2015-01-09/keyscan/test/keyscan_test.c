#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include "ks_api.h"

#define DEV_NAME    "/dev/ks_dev"

/* GM8210 Demo board use below gpio number for 18 buttons keypad design */
//#define GM_8287_EVB 
#define GM_8210_EVB

#if defined(GM_8210_EVB)
#define KEY_OUT_0 0
#define KEY_OUT_1 1
#define KEY_OUT_2 2

#define KEY_IN_0 5
#define KEY_IN_1 6
#define KEY_IN_2 7
#elif defined(GM_8287_EVB)

#define KEY_OUT_0 20
#define KEY_OUT_1 24
#define KEY_OUT_2 17

#define KEY_IN_0 18
#define KEY_IN_1 21
#define KEY_IN_2 19

#endif

#define GPIO_BIT(x)  (1 << x)


int main(int argc, char *argv[])
{
    int keyscan_fd;
	int ret;
	char user_key;
	int key_code = -1;
	unsigned int i, j, key_found = 0;
    struct timeval tv;
    fd_set rd;
	
        /*gpio map to bit          KEY_IN_0,           KEY_IN_1,          KEY_IN_2,  */
	unsigned int key_i[] = {GPIO_BIT(KEY_IN_0), GPIO_BIT(KEY_IN_1), GPIO_BIT(KEY_IN_2)
                            , GPIO_BIT(KEY_IN_0) | GPIO_BIT(KEY_IN_1)   /*KEY_IN_0 | KEY_IN_1*/
                            , GPIO_BIT(KEY_IN_1) | GPIO_BIT(KEY_IN_2)   /*KEY_IN_1 | KEY_IN_2*/
                            , GPIO_BIT(KEY_IN_0) | GPIO_BIT(KEY_IN_2)}; /*KEY_IN_0 | KEY_IN_2*/

	unsigned int key_o[] = {KEY_OUT_2, KEY_OUT_1, KEY_OUT_0};
	unsigned int gpio_i, gpio_o;
	unsigned int gpio_i_set = sizeof(key_i)/sizeof(key_i[0]);
	unsigned int gpio_o_set = sizeof(key_o)/sizeof(key_o[0]);
	unsigned int const exit_but = gpio_i_set * gpio_o_set;

    keyscan_pub_data  data;

    printf("Start KEYSCAN Test...\n");
    printf("gpio_i_set = %d gpio_o_set = %d \n",gpio_i_set,gpio_o_set);

    keyscan_fd = open(DEV_NAME, O_RDONLY|O_NONBLOCK);

    if(keyscan_fd<0)
    {
        printf("Can't open %s.\n", DEV_NAME);
        return -1;
    }
    
    while (1)
    {
        tv.tv_sec = 0;
        tv.tv_usec = 100000; /* timeout 100 ms */
        FD_ZERO(&rd);
        FD_SET(keyscan_fd, &rd);

        select(keyscan_fd+1, &rd, NULL, NULL, &tv);

        if(read(keyscan_fd, &data, sizeof(keyscan_pub_data))>0)
        {
        	gpio_i = data.key_i;
			gpio_o = data.key_o;
			key_found = 0;
			for (i = 0; i<gpio_o_set; i++)
			{
				if (gpio_o == key_o[i])
				{
					for (j = 0; j< gpio_i_set; j++)
					{
						if (gpio_i == key_i[j])
						{
							key_found = 1;
							break;
						}
					}
                    if(key_found)
                        break;
				}
			}

            if (key_found)
            {
#if defined(GM_8210_EVB)            
            	key_code = j*gpio_o_set + i + 1;
#elif defined(GM_8287_EVB)
				key_code = i*gpio_i_set + j + 1;
#endif
                printf("Keyscan... PB%d] is detected. %s\n", key_code,(data.status==KEY_REPEAT)?"(Repeat)":" ");
            }
            else
                printf("Keyscan... command[0x%x, 0x%x, Unknown] is detected. \n", gpio_i, gpio_o);
			
            if (key_code == exit_but)
            {
                printf("Exit? (Y/N)\n");
                scanf("%c", &user_key);
                if (user_key=='y' || user_key=='Y')
                {
                    break;
                }
            }
        }
    }

    close(keyscan_fd);
    return 0;
}




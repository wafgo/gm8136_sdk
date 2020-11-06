#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>
#include "irda_api.h"

#define DEV_NAME    "/dev/irda_dev"

struct cmd_info {
    int     val;
    char    *text;
};

#define CMD_COUNT   27
struct cmd_info cmd_map[CMD_COUNT] =
{
    {0xff00ff00, "power"},
    {0xfc03ff00, "mute"},
    {0xfb04ff00, "tv-mode"},
    {0xf708ff00, "osd"},
    {0xfd02ff00, "angle"},
    {0xf609ff00, "zoom"},
    {0xf00fff00, "setup"},
    {0xfa05ff00, "up"},
    {0xf807ff00, "play/pause"},
    {0xf30cff00, "left"},
    {0xf20dff00, "ok"},
    {0xf50aff00, "right"},
    {0xf10eff00, "stop/return"},
    {0xee11ff00, "down"},
    {0xec13ff00, "aspect-ratio"},
    {0xeb14ff00, "audio"},
    {0xea15ff00, "next"},
    {0xe916ff00, "forward"},
    {0xe817ff00, "vol-up"},
    {0xe718ff00, "subtitle"},
    {0xe619ff00, "previous"},
    {0xe51aff00, "rewind"},
    {0xe41bff00, "vol-down"},
    {0xe31cff00, "photot"},
    {0xe21dff00, "music"},
    {0xe11eff00, "movie"},
    {0xe01fff00, "preview"}
};

#define GPIO_NUM 3

typedef struct _GPIO_PIN {
	unsigned int port[GPIO_NUM];
	unsigned int value[GPIO_NUM];
} gpio_pin;

#define GPIO_DEV_NAME "/dev/ftgpio_function"
#define BIT(x) (1<<x)
#define GPIO_IOC_MAGIC		'g'
#define GPIO_SET_MULTI_PINS_OUT		_IOW(GPIO_IOC_MAGIC, 1, gpio_pin)
#define GPIO_SET_MULTI_PINS_IN		_IOW(GPIO_IOC_MAGIC, 2, gpio_pin)
#define GPIO_GET_MULTI_PINS_VALUE	_IOW(GPIO_IOC_MAGIC, 3, gpio_pin)
#define GPIO_IOC_MAXNR 3




int main(int argc, char *argv[])
{
    int irdet_fd, ret, len, i;
	char user_key;
    irda_pub_data  data;
    struct timeval tv;
    fd_set rd;


    printf("Start IRDET test...\n");
    
    irdet_fd = open(DEV_NAME, O_RDONLY|O_NONBLOCK);
    if(irdet_fd<0)
    {
        printf("Can't open %s.\n", DEV_NAME);
        return -1;
    }

	//trig_gpio();
    while(1)
    {

        tv.tv_sec = 1;/* timeout  1 s */
        tv.tv_usec = 0; 
        FD_ZERO(&rd);
        FD_SET(irdet_fd, &rd);

        select(irdet_fd+1, &rd, NULL, NULL, &tv);
        len = read(irdet_fd, &data, sizeof(irda_pub_data));
		if (len == sizeof(irda_pub_data))
        {
            for(i=0; i<CMD_COUNT; i++)
                if(data.val == cmd_map[i].val)
                    break;
            if(i<CMD_COUNT)
                printf("Irdet... command[0x%x, ""%s""] is detected. %s\n", data.val, cmd_map[i].text, data.repeat?"(Repeat)":" ");
            else
                printf("Irdet... command[0x%x, Unknown] is detected. \n", data.val, cmd_map[i].text);
            if(i==0)
            {
                printf("Exit? (Y/N)\n");
                scanf("%c", &user_key);
                if(user_key=='y' || user_key=='Y')
                    break;
            }
        }
    }

    close(irdet_fd);
    return 0;
}




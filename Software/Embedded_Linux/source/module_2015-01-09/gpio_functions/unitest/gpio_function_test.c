#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ioctl_gpio.h"

#define GPIO_DEV_NAME "/dev/ftgpio_function"
#define BIT(x) (1<<x)

int main(int argc, char *argv[])
{
    int fd = -1;
    int ret = 0;
    gpio_pin pin;
    memset(&pin, 0, sizeof(pin));
    fd = open(GPIO_DEV_NAME, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Can't open /dev/ftgpio_function\n");
        return -1;
    }
    // set GPIO_0 pin 16, 17, 18 as 1, 1, 0
    pin.port[0] |= BIT(16) | BIT(17) | BIT(18);
    pin.value[0] |= BIT(16) | BIT(17);

#if 1
    ret = ioctl(fd, GPIO_SET_MULTI_PINS_OUT, &pin);
    if (ret < 0) {
        perror("IOCTL err!\n");
        return -1;
    }
#endif
#if 0
    ret = ioctl(fd, GPIO_SET_MULTI_PINS_IN, &pin);
    if (ret < 0) {
        perror("IOCTL err!\n");
        return -1;
    }
#endif
    ret = ioctl(fd, GPIO_GET_MULTI_PINS_VALUE, &pin);
    if (ret < 0) {
        perror("IOCTL err!\n");
        return -1;
    }
    printf("pin.value[0] = 0x%X\n", pin.value[0]);
    close(fd);
    return 0;
}

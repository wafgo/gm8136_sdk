#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include "lcd_def.h"

struct cursor_info_s {
    int xres;
    int yres;
    int xpos;
    int ypos;
    void *priv_data;
} cursor_info[LCD_IP_NUM];

static int init_done = 0;

int lcd_update_cursor_info(int target_lcd_idx, int xres, int yres, int xpos, int ypos, void *priv_data)
{
    if (!init_done) {
        init_done = 1;
        memset((void *)&cursor_info[0], 0, sizeof(struct cursor_info_s) * LCD_IP_NUM);
    }
    if (target_lcd_idx > LCD_IP_NUM)
        return -1;

    cursor_info[target_lcd_idx].xres = xres;
    cursor_info[target_lcd_idx].yres = yres;
    cursor_info[target_lcd_idx].xpos = xpos;
    cursor_info[target_lcd_idx].ypos = ypos;
    cursor_info[target_lcd_idx].priv_data = priv_data;

    return 0;
}

int lcd_get_cursor_info(int my_lcd_idx, int *xres, int *yres, int *xpos, int *ypos, void *priv_data)
{
    if (!init_done) {
        init_done = 1;
        memset((void *)&cursor_info[0], 0, sizeof(struct cursor_info_s) * LCD_IP_NUM);
    }
    if (my_lcd_idx > LCD_IP_NUM)
        return -1;

    *xres = cursor_info[my_lcd_idx].xres;
    *yres = cursor_info[my_lcd_idx].yres;
    *xpos = cursor_info[my_lcd_idx].xpos;
    *ypos = cursor_info[my_lcd_idx].ypos;
    *(u32 *)priv_data = (u32)cursor_info[my_lcd_idx].priv_data;

    return 0;
}

EXPORT_SYMBOL(lcd_update_cursor_info);
EXPORT_SYMBOL(lcd_get_cursor_info);

MODULE_LICENSE("GPL");

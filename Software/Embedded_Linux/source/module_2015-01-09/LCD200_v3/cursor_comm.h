#ifndef __CURSOR_COMM_H__
#define __CURSOR_COMM_H__

/* lcd_update_cursor_info: set the main screen cursor postion to the target LCD, ...
 * Return: 0 for success, -1 for fail
 */
int lcd_update_cursor_info(int target_lcd_idx, int xres, int yres, int xpos, int ypos, void *priv_data);

/* lcd_get_cursor_info: get the main screen cursor postion, ...
 * Return: 0 for success, -1 for fail
 */
int lcd_get_cursor_info(int my_lcd_idx, int *xres, int *yres, int *xpos, int *ypos, void *priv_data);

#endif /* __CURSOR_COMM_H__ */
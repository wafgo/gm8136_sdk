#ifndef _VCAP_PALETTE_H_
#define _VCAP_PALETTE_H_

#define VCAP_PALETTE_COLOR_AQUA              0x4893CA        /* CrCbY */
#define VCAP_PALETTE_COLOR_BLACK             0x808010
#define VCAP_PALETTE_COLOR_BLUE              0x6ef029
#define VCAP_PALETTE_COLOR_BROWN             0xA15B51
#define VCAP_PALETTE_COLOR_DODGERBLUE        0x3FCB69
#define VCAP_PALETTE_COLOR_GRAY              0x8080B5
#define VCAP_PALETTE_COLOR_GREEN             0x515B51
#define VCAP_PALETTE_COLOR_KHAKI             0x894872
#define VCAP_PALETTE_COLOR_LIGHTGREEN        0x223690
#define VCAP_PALETTE_COLOR_MAGENTA           0xDECA6E
#define VCAP_PALETTE_COLOR_ORANGE            0xBC5198
#define VCAP_PALETTE_COLOR_PINK              0xB389A5
#define VCAP_PALETTE_COLOR_RED               0xF05A52
#define VCAP_PALETTE_COLOR_SLATEBLUE         0x60A63D
#define VCAP_PALETTE_COLOR_WHITE             0x8080EB
#define VCAP_PALETTE_COLOR_YELLOW            0x9210D2

typedef enum {
    WHITE_IDX = 0,
    BLACK_IDX,
    RED_IDX,
    BLUE_IDX,
    YELLOW_IDX,
    GREEN_IDX,
    BROWN_IDX,
    DODGERBLUE_IDX,
    GRAY_IDX,
    KHAKI_IDX,
    LIGHTGREEN_IDX,
    MAGENTA_IDX,
    ORANGE_IDX,
    PINK_IDX,
    SLATEBLUE_IDX,
    AQUA_IDX,
    COLOR_IDX_MAX
} VCAP_COLOR_T;

int vcap_palette_init(struct vcap_dev_info_t *pdev_info);
int vcap_palette_set(struct vcap_dev_info_t *pdev_info, int id, u32 crcby);
int vcap_palette_get(struct vcap_dev_info_t *pdev_info, int id, u32 *crcby);

#endif  /* _VCAP_PALETTE_H_ */

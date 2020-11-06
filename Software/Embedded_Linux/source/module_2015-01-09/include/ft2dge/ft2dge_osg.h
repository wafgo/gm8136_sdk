#ifndef __FT2DGE_OSG_H__
#define __FT2DGE_OSG_H__

typedef struct __ft2d_osg_blit{
    unsigned int       dst_paddr;
    unsigned short     dst_w;
    unsigned short     dst_h;
    unsigned int       src_paddr;
    unsigned short     src_w;
    unsigned short     src_h;
    unsigned short     dx;
    unsigned short     dy;
    unsigned int       src_colorkey;
}ft2d_osg_blit_t;

typedef struct __ft2d_osg_mask{
    unsigned int       dst_paddr;
    unsigned short     dst_w;
    unsigned short     dst_h;
    unsigned short     dx1;
    unsigned short     dy1;
    unsigned short     dx2;
    unsigned short     dy2;
    unsigned int       color;
    unsigned int       type;
    unsigned int       border_sz;
}ft2d_osg_mask_t;

typedef enum {
    FT2D_OSG_BORDER_TURE   = 1,
    FT2D_OSG_BORDER_HORROW = 2   
} FT2D_OSG_BORDER_TP;

int ft2d_osg_do_blit(ft2d_osg_blit_t* blit_pam,int fire);

int ft2d_osg_do_mask(ft2d_osg_mask_t* mask_pam,int fire);

int ft2d_osg_remainder_fire(void);

#endif /* __FT2DGE_IF_H__ */

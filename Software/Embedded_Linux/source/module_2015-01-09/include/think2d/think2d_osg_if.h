#ifndef _THINK2D_OSG_IF_H_
#define _THINK2D_OSG_IF_H_

typedef struct __think2d_osg_blit{
    unsigned int       dst_paddr;
    unsigned short     dst_w;
    unsigned short     dst_h;
    unsigned int       src_paddr;
    unsigned short     src_w;
    unsigned short     src_h;
    unsigned short     dx;
    unsigned short     dy;
    unsigned int       src_colorkey;
}think2d_osg_blit_t;

typedef struct __think2d_osg_mask{
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
}think2d_osg_mask_t;

typedef enum {
    THINK2D_0SG_BORDER_TURE   = 1,
    THINK2D_0SG_BORDER_HORROW = 2   
} THINK2D_OSG_BORDER_TP;

int think2d_osg_do_blit(think2d_osg_blit_t* blit_pam,int fire);

int think2d_osg_do_mask(think2d_osg_mask_t* mask_pam,int fire);

int think2d_osg_remainder_fire(void);

#endif

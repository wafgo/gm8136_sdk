/*
 * Note: The following just lists the input resolution description. For the real supported resolution in platfrom,
 * please check platform related files such as gm8181.c, gm8185.c ...
 */

static ffb_input_res_t ffb_input_res[] = {
    {
     .input_res = VIN_D1,
#ifdef D1_WIDTH_704
     .xres = 704,
#else
     .xres = 720,
#endif
     .yres = 480,
     .name = "NTSC"},
    {
     /* PAL */
     .input_res = VIN_PAL,
#ifdef D1_WIDTH_704
     .xres = 704,
#else
     .xres = 720,
#endif
     .yres = 576,
     .name = "PAL"},
    /* VGA */
    {
     .input_res = VIN_640x480,
     .xres = 640,
     .yres = 480,
     .name = "640x480(VGA)"},
    /* SVGA */
    {
     .input_res = VIN_800x600,
     .xres = 800,
     .yres = 600,
     .name = "800x600(SVGA)"},
    /* XVGA */
    {
     .input_res = VIN_1024x768,
     .xres = 1024,
     .yres = 768,
     .name = "1024x768(XVGA)"},
    /* HD, not standard */
    {
     .input_res = VIN_1440x1080,
     .xres = 1440,
     .yres = 1080,
     .name = "1440x1080(HD)"},
    /* WXGA */
    {
     .input_res = VIN_1280x800,
     .xres = 1280,
     .yres = 800,
     .name = "1280x800(WXGA)"},
    /* 1280x960 */
    {
     .input_res = VIN_1280x960,
     .xres = 1280,
     .yres = 960,
     .name = "1280x960"},
    /* SXGA */
    {
     .input_res = VIN_1280x1024,
     .xres = 1280,
     .yres = 1024,
     .name = "1280x1024(SXGA)"},
    /* SXGA */
    {
     .input_res = VIN_1360x768,
     .xres = 1360,
     .yres = 768,
     .name = "1360x768(HD)"},
    /* 1600x1200 */
    {
     .input_res = VIN_1600x1200,
     .xres = 1600,
     .yres = 1200,
     .name = "1600x1200"},
    /* 720P */
    {
     .input_res = VIN_720P,
     .xres = 1280,
     .yres = 720,
     .name = "1280x720(720P)"},
    /* Full HD */
    {
     .input_res = VIN_1920x1080,
     .xres = 1920,
     .yres = 1080,
     .name = "1920x1080"},
    /* 640x360 */
    {
     .input_res = VIN_640x360,
     .xres = 640,
     .yres = 360,
     .name = "640x360"},
    /* 1680x1050 */
    {
     .input_res = VIN_1680x1050,
     .xres = 1680,
     .yres = 1050,
     .name = "1680x1050(SXGA)"},
    /* 1440x1152 */
    {
     .input_res = VIN_1440x1152,
     .xres = 1440,
     .yres = 1152,
     .name = "1440x1152"},
    /* 1440x960 */
    {
     .input_res = VIN_1440x960,
     .xres = 1440,
     .yres = 960,
     .name = "1440x960"},
    /* 1440x900 */
    {
     .input_res = VIN_1440x900,
     .xres = 1440,
     .yres = 900,
     .name = "1440x900"},
    /* 540x360 */
    {
     .input_res = VIN_540x360,
     .xres = 540,
     .yres = 360,
     .name = "540x360"},
     /* 352x240 */
    {
     .input_res = VIN_352x240,
     .xres = 352,
     .yres = 240,
     .name = "352x240"},
     /* 352x288 */
    {
     .input_res = VIN_352x288,
     .xres = 352,
     .yres = 288,
     .name = "352x288"},
     /* 320x240 */
    {
     .input_res = VIN_320x240,
     .xres = 320,
     .yres = 240,
     .name = "320x240"},
    /* 1280x1080 */
    {
     .input_res = VIN_1280x1080,
     .xres = 1280,
     .yres = 1080,
     .name = "1280x1080"},
     /* 1920x1152 */
    {
     .input_res = VIN_1920x1152,
     .xres = 1920,
     .yres = 1152,
     .name = "1920x1152"},
};

ffb_input_res_t *ffb_inres_get_info(VIN_RES_T input)
{
    int i;
    ffb_input_res_t *entry = NULL;

    for (i = 0; i < ARRAY_SIZE(ffb_input_res); i++) {
        if (input == ffb_input_res[i].input_res) {
            entry = &ffb_input_res[i];
            break;
        }
    }

    return entry;
}

//EXPORT_SYMBOL(ffb_inres_get_info);
//MODULE_LICENSE("GPL");

/* A map table used to be query
 */
enum property_id {
    ID_VIDEOGRAPH_PLANE = MAX_WELL_KNOWN_ID_NUM + 1, /* which plane runs videograph */
};

struct property_map_t {
    int id;
    char *name;
    char *readme;
} property_map[] = {    
    {ID_SRC_FRAME_RATE, "src_frame_rate", "source frame rate, such as 60fps"},
    {ID_VIDEOGRAPH_PLANE, "videograph_plane", "which videoplane runs videograph"},
    {ID_DST_DIM, "dst_dim", "the dimension of destination"},  //width<<16|height
    {ID_SRC_BG_DIM, "src_bg_dim", "the background dimension of the source"},  //width<<16|height
    {ID_SRC_DIM, "src_dim", "source dimension"},    //width<<16|height
    {ID_SRC_XY, "src_xy", "xy offset to the source background"},  //x<<16|y
    {ID_SRC_FMT, "src_fmt", "source format"},
};

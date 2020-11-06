#ifndef _FDT_H_
#define _FDT_H_


#define FDT_VER               0x00000400
//                              HIIFFFBB
//                              H         huge change
//                               II       interface change
//                                 FFF    functional modified or bug fixed
//                                    BB  branch for customer request

#define FDT_INT_VER           0x14123100     //    added for internal release only
//                              YYMMDDSS
//                              YY         year
//                                MM         month
//                                  DD     date
//                                    SS     sequence number

#define FDT_VER_MAJOR         ((FDT_VER >> 28) & 0x0f)   // huge change
#define FDT_VER_MINOR         ((FDT_VER >> 20) & 0xff)   // interface change
#define FDT_VER_MINOR2        ((FDT_VER >> 8) & 0xfff)   // functional modified or bug fixed
#define FDT_VER_BRANCH        ((FDT_VER >> 0) & 0x0ff)   // branch for customer request

#define FDT_INIT              0x4190  ///< IOCTL command ID to initialize face detection.
#define FDT_DETECT            0x4191  ///< IOCTL command ID to detect faces in a captured frame.
#define FDT_END               0x4192  ///< IOCTL command ID to end face detection.

#define FDT_DEV               "/dev/fdt"


/// Structure of anti-variation parameters in face detection module.
typedef struct _fdt_av_param_t {
    char en;                     ///< Enables anti-variation to decrease flicker of face rectangles. 1: enable. 0: disable.
    char delayed_vanish_frm_num; ///< A face rectangle is removed if it's not detected for continuous \delayed_vanish_frm_num frames. Range: 0 ~ 127.
    char min_frm_num;            ///< Minimum number of watched frames in anti-variation. Range: 1 ~ 16.
    char max_frm_num;            ///< Maximum number of watched frames in anti-variation. Range: 1 ~ 16.
} fdt_av_param_t;

/// Structure of face detection parameters.
typedef struct _fdt_param_t {
    int cap_vch;                 ///< User channel number.
    unsigned int ratio;          ///< Scaling-down ratio. Range: 80 ~ 91 (for 80% ~ 91%).
    unsigned int first_width;    ///< The width of first scaled frame. Must be multiple of 4. Range: 64 ~ 320.
    unsigned int first_height;   ///< The height of first scaled frame. Must be multiple of 2. Range: 36 ~ 320.
    int scaling_timeout_ms;      ///< Scaler timeout duration in millisecond. Range: > 100.
    unsigned int source_width;   ///< The source image width.
    unsigned int source_height;  ///< The source image height.
    unsigned char sensitivity;   ///< Controls the detection sensitivity. Ranges: 0 ~ 9.
    fdt_av_param_t av_param;     ///< Anti-variation parameters.

    unsigned short max_face_num; ///< Maximum number of detected faces. Range: 0 ~ 65535.
    unsigned short face_num;     ///< Output: Detected face number.
    unsigned short *faces;       ///< Output: Detected face rectangle positions and sizes in the scale of source image size.
                                 ///< This buffer MUST be allocated by AP with size 8 * \max_face_num in bytes.
                                 ///< Let n be an integer ranging from 0 to face_num-1, then
                                 ///<   \faces[4n] is the x position of the n-indexed face rectangle;
                                 ///<   \faces[4n + 1] is the y position of the n-indexed face rectangle;
                                 ///<   \faces[4n + 2] is the width of the n-indexed face rectangle;
                                 ///<   \faces[4n + 3] is the height of the n-indexed face rectangle.

    void *rt;                    ///< Runtime buffer. Internally managed by face detection driver.
} fdt_param_t;


#endif /* _FDT_H_ */

#ifndef _FDT_ALG_H_
#define _FDT_ALG_H_

/// Structure of an object group.
typedef struct _TObjGrp {
    int count; ///< Number of objects in this group.
    int x;     ///< Center x coordinate of the group.
    int y;     ///< Center y coordinate of the group.
    int w;     ///< Width of the group.
    int sx;    ///< Sum of x coordinates of objects in this group.
    int sy;    ///< Sum of y coordinates of objects in this group.
    int sw;    ///< Sum of widths of objects in this group.
} TObjGrp;

/// The structure of a rectangle
typedef struct _TRect {
    unsigned short x, y,          ///< left-top corner coordinate of the rectangle
                   width, height; ///< width and height of the rectangle
} TRect;

/// The structure of a 2D point
typedef struct _TPoint2D {
    unsigned short x, y;
} TPoint2D;

/// The structure of anti-variation parameters.
typedef struct _TAntiVarParam {
    int thMissDetectCount;   ///< Threshold for identifying if the object truly disappears.
    int thMoveRatio;         ///< Threshold for identifying if a detected object is moving.
                             ///< Represented by the ratio of the last stabilized object width:
                             ///<   threshold = (lastStbWidth * thMoveRatio) / 128
    int thAbnormalMoveRatio; ///< Threshold for identifying if a detected object is moving abnormally.
                             ///< Represented by the ratio of the last stabilized object width:
                             ///<   threshold = (lastStbWidth * thAbnormalMoveRatio) / 128
    int thAbnormalMoveCount; ///< Threshold for confirming if the object width is in transition state.
    int thWidthChangeRatio;  ///< Threshold for identifying if a detected object width is changing.
                             ///< Represented by the ratio of the last stabilized object width:
                             ///<   threshold = (lastStbWidth * thWidthChangeRatio) / 128
    int thWidthChangeCount;  ///< Threshold for confirming if the object width is in transition state.
    int minMedianWinSize;    ///< Minimum median window size. Range: 1 ~ 16.
    int maxMedianWinSize;    ///< Maximum median window size. Range: 1 ~ 16.
} TAntiVarParam;

/// The private context of an object under anti-variation.
typedef struct _TAntiVarCtx {
    #define MEDIAN_BUF_SIZE  (16)
    TPoint2D lastCenter;     ///< Last center of the object rectangle.
    int missDetectCount;     ///< Count for miss detection.
    int abnormalMoveCount;   ///< Count for abnormal movement times.
    int widthHistory[MEDIAN_BUF_SIZE];       ///< History of rectangle widths (internally used only).
    int sortedHistory[MEDIAN_BUF_SIZE];      ///< Sorted history of rectangle widths (internally used only).
    int medianWinSize;       ///< Window size of median filter for size stabilization of a detected object (internally used only).
    int count;               ///< Internal counter (internally used only).
    int widthChangeCount;    ///< Counting for width changing.
    int widthChangeDir;      ///< Direction of width changing. 1: increasing, 0: stable, and -1: decreasing.
    int lastStbWidth;        ///< Last stabilized object width.
} TAntiVarCtx;

/// The anti-variation runtime variables.
typedef struct _TAntiVarRT {
    int maxObjs;       ///< Maximum number of logged objects.
    int objs;          ///< Number of logged objects.
    TAntiVarCtx *ctx;  ///< Anti-variation context of each object.
    int *distMat;      ///< Distance matrix saving distances between logged objects and detected objects.
    int *detectedOrphan;
    int *loggedOrphan;
    int *rmIdx;
} TAntiVarRT;

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                                  PROTOTYPES                                //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

extern unsigned short OD_ObjectGrouper(TRect *pData, const unsigned short srcFaceCount,
                                       unsigned char merge_min,
                                       unsigned char gp_dist,
                                       unsigned char ov_dist1,
                                       unsigned char ov_dist2,
                                       TObjGrp groups[],
                                       int maxObjects);

extern int OD_CreateAntiVariationRT(TAntiVarRT *pRT, const int maxObjectNum);

/** Stabilizes multiple object rectangles.
 * @param[in,out] objectRects    Array of detected object rectangles.
 * @param[in,out] objectNum      Pointer to the number of detected object rectangles.
 * @param[in]     pParam         Pointer to the parameters.
 * @param[in]     pRT            Pointer to the runtime.
 */
extern void OD_AntiVariation(TRect *faceRects, int *objectNum,
                             TAntiVarParam *pParam,
                             TAntiVarRT *pRT);
extern void OD_DestroyAntiVariationRT(TAntiVarRT *pRT);

#endif /* _FDT_ALG_H_ */

///#define _DEBUG_
#define _AV_OUTPUT_LOGGED_ORDER_

#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/sort.h>
#include "fdt_alg.h"


////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                              LOCAL FUNCTIONS                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

int compare(const void *val_1, const void *val_2)
{
   return (*(int*)val_1) - (*(int*)val_2) ;
}


/** Stabilizes one object rectangle.
 * @param[in,out] pRect     Pointer to the object rectangle.
 * @param[in]     pParam    Pointer to the parameters.
 * @param[in]     pContext  Pointer to context of the anti-variation job.
 */
void stabilizeOneObject(TRect *pRect, TAntiVarParam *pParam, TAntiVarCtx *pContext)
{
    int thMove, thAbnMove, thAbnMoveCnt, d2, abnMoveCnt;
    const int thWidthChangeCount = pParam->thWidthChangeCount,
              maxMedianWinSize = pParam->maxMedianWinSize;
    int thWidthChange,
        medianWinSize,
        widthChangeCount,
        widthChangeDir,
        lastStbWidth;
    int *width_quanti_record = pContext->widthHistory,
        *width_for_sort = pContext->sortedHistory,
        count;
    TRect *face_rect;
    int c_x_detect = 0, c_y_detect = 0,
        c_x_record = 0, c_y_record = 0,
        c_x_stable = 0, c_y_stable = 0,
        d_x = 0, d_y = 0, width_stable = 0;

    // Copy context
    c_x_record = pContext->lastCenter.x;
    c_y_record = pContext->lastCenter.y;
    medianWinSize = pContext->medianWinSize;
    count = pContext->count;
    widthChangeCount = pContext->widthChangeCount,
    widthChangeDir = pContext->widthChangeDir,
    lastStbWidth = pContext->lastStbWidth;
    abnMoveCnt = pContext->abnormalMoveCount;

    face_rect = pRect;
    // Derive face center
    c_x_detect = face_rect->x + (face_rect->width>>1);
    c_y_detect = face_rect->y + (face_rect->height>>1);

    thMove = (lastStbWidth * pParam->thMoveRatio) >> 7;
    thMove *= thMove;
    thAbnMove = (lastStbWidth * pParam->thAbnormalMoveRatio) >> 7;
    thAbnMove *= thAbnMove;
    thAbnMoveCnt = pParam->thAbnormalMoveCount;

    //-----------------------
    // Location stabilization
    //-----------------------
    d_x = c_x_detect - c_x_record;
    d_y = c_y_detect - c_y_record;
    d2 = (d_x * d_x) + (d_y * d_y); // distance square
    if (d2 <= thMove) {
        // smooth the movement
        c_x_stable = (c_x_record * 14 + c_x_detect * 2) >> 4; // c_x_record * 0.9 + c_x_detect * 0.1
        c_y_stable = (c_y_record * 14 + c_y_detect * 2) >> 4; // c_x_record * 0.9 + c_x_detect * 0.1
        abnMoveCnt = 0;
    }
    else if (d2 <= thAbnMove) {
        c_x_stable = c_x_detect;
        c_y_stable = c_y_detect;
        abnMoveCnt = 0;
    }
    else {
        // abnormal movement
        if (abnMoveCnt > thAbnMoveCnt) {
            c_x_stable = c_x_detect;
            c_y_stable = c_y_detect;
            abnMoveCnt = 0;
        }
        else {
            c_x_stable = c_x_record;
            c_y_stable = c_y_record;
            ++abnMoveCnt;
        }
    }

    //-----------------------
    // Size stabilization
    //-----------------------

    // Quantization of face_rect width (set to multiples of 8)
    face_rect->width = (((face_rect->width + 4) >> 3) << 3);

    // Sorting and taking the median value
    if (count < maxMedianWinSize)
    {
        width_quanti_record[count] = face_rect->width;
        width_stable = face_rect->width;
        lastStbWidth = width_stable;
    }
    else
    {
        int deltaWidth = lastStbWidth - face_rect->width;
        thWidthChange = ((lastStbWidth * pParam->thWidthChangeRatio) >> 7);
        if ((deltaWidth > thWidthChange) || (-deltaWidth > thWidthChange)) {
            if (deltaWidth > 0) {
                if (widthChangeDir <= 0) {
                    // width is increasing
                    widthChangeCount = 1;
                    widthChangeDir = 1;
                }
                else
                    ++widthChangeCount;
            }
            else if (deltaWidth < 0) {
                if (widthChangeDir >= 0) {
                    // width is decreasing
                    widthChangeCount = 1;
                    widthChangeDir = -1;
                }
                else
                    ++widthChangeCount;
            }
        }
        else if (widthChangeCount > 0)
            --widthChangeCount;
        else
            widthChangeDir = 0;
        if (widthChangeCount >= thWidthChangeCount)
            medianWinSize = pParam->minMedianWinSize;
        else if (medianWinSize < maxMedianWinSize)
            ++medianWinSize;

        // update width history
        memmove(width_quanti_record, width_quanti_record+1, sizeof(int)*(maxMedianWinSize-1));
        width_quanti_record[maxMedianWinSize-1] = face_rect->width;
        // get median width
        memcpy(width_for_sort, &width_quanti_record[maxMedianWinSize - medianWinSize], sizeof(int)* medianWinSize);
        sort(width_for_sort, medianWinSize, sizeof(int), compare, NULL);
        if (medianWinSize % 2 == 1)
        {
            width_stable = width_for_sort[medianWinSize>>1];
        }
        else
        {
            width_stable = (width_for_sort[(medianWinSize>>1)] + width_for_sort[(medianWinSize>>1)-1]) >> 1;
        }
        lastStbWidth = width_stable;
    }
    count++;

    //-----------------------------------
    // Assign stable values to face_rect
    //-----------------------------------
    face_rect->x = c_x_stable - (width_stable>>1);
    face_rect->y = c_y_stable - (width_stable>>1);
    face_rect->width = width_stable;
    face_rect->height = width_stable;

    //-----------------------------------
    // Update the history recording
    //-----------------------------------
    pContext->abnormalMoveCount = abnMoveCnt;
    pContext->lastCenter.x = c_x_stable;
    pContext->lastCenter.y = c_y_stable;
    pContext->count = count;
    pContext->medianWinSize = medianWinSize;
    pContext->widthChangeCount = widthChangeCount;
    pContext->widthChangeDir = widthChangeDir;
    pContext->lastStbWidth = lastStbWidth;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                              GLOBAL FUNCTIONS                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

unsigned short OD_ObjectGrouper(TRect *pData, const unsigned short srcFaceCount,
                                unsigned char merge_min,
                                unsigned char gp_dist,
                                unsigned char ov_dist1,
                                unsigned char ov_dist2,
                                TObjGrp groups[],
                                int maxObjects)
{
    int i;
    int j;
    int i_rect, i_comp;
    int total_comp = 0;
    int total_valid_comp = 0;
    int tarFaceCount = 0;
    TRect* srcFaceSet = (TRect*)pData;
    TObjGrp *pGroup;
    TRect *pRect;
    TRect *pRect2;
    int dist;
    int dist2;
    bool grouped;

    // 1. Group similar windows
    for (i_rect = 0; i_rect < srcFaceCount; i_rect++) {
        grouped = false;
        // represent rect position by its center
        srcFaceSet[i_rect].x += (srcFaceSet[i_rect].width >> 1);
        srcFaceSet[i_rect].y += (srcFaceSet[i_rect].width >> 1);

        for (i_comp = 0; i_comp < total_comp; i_comp++) {
            pGroup = &groups[i_comp];
            pRect = &srcFaceSet[i_rect];
            dist = pGroup->w >> gp_dist;

            if (pRect->x <= pGroup->x + dist &&
                pRect->x >= pGroup->x - dist &&
                pRect->y <= pGroup->y + dist &&
                pRect->y >= pGroup->y - dist &&
                pRect->width <= (pGroup->w + dist) &&
                (pRect->width + dist) >= pGroup->w) {

                ++pGroup->count;
                pGroup->sx += pRect->x;
                pGroup->sy += pRect->y;
                pGroup->sw += pRect->width;
                pGroup->x = (pGroup->sx + (pGroup->count >> 1)) / pGroup->count;
                pGroup->y = (pGroup->sy + (pGroup->count >> 1)) / pGroup->count;
                pGroup->w = (pGroup->sw + (pGroup->count >> 1)) / pGroup->count;

                grouped = true;
                break;
            }
        }

        if (total_comp < maxObjects && !grouped) {
            groups[total_comp].sx = srcFaceSet[i_rect].x;
            groups[total_comp].sy = srcFaceSet[i_rect].y;
            groups[total_comp].sw = srcFaceSet[i_rect].width;
            groups[total_comp].x = srcFaceSet[i_rect].x;
            groups[total_comp].y = srcFaceSet[i_rect].y;
            groups[total_comp].w = srcFaceSet[i_rect].width;
            groups[total_comp].count = 1;
            total_comp++;
        }
    }

    // 2. Prune orphan groups
    for (i_comp = 0; i_comp < total_comp; i_comp++) {
        if (groups[i_comp].count >= merge_min) {
            srcFaceSet[total_valid_comp].x = groups[i_comp].x;
            srcFaceSet[total_valid_comp].y = groups[i_comp].y;
            srcFaceSet[total_valid_comp].width = groups[i_comp].w;
            srcFaceSet[total_valid_comp].height = groups[i_comp].count;
            total_valid_comp++;
        }
    }

    // 3. Filter out overlapping groups
    for (i = 0; i < total_valid_comp; i++) {
        int flag = 1;

        pRect = &srcFaceSet[i];
        if (pRect->height == 0)
            continue;

        for (j = i + 1; j < total_valid_comp; j++) {
            pRect2 = &srcFaceSet[j];
            if (pRect2->height == 0)
                continue;
            dist2 = pRect2->width >> ov_dist2;
            dist = pRect->width >> ov_dist1;

            if (
                (pRect->x >= pRect2->x - dist2 &&
                pRect->y >= pRect2->y - dist2 &&
                pRect->x + pRect->width <= pRect2->x + pRect2->width + dist2 &&
                pRect->y + pRect->width <= pRect2->y + pRect2->width + dist2) ||
                (pRect2->x >= pRect->x - dist &&
                pRect2->y >= pRect->y - dist &&
                pRect2->x + pRect2->width <= pRect->x + pRect->width + dist &&
                pRect2->y + pRect2->width <= pRect->y + pRect->width + dist)) {
                if (pRect2->height >= pRect->height) {
                    flag = 0;
                    break;
                }
                else {
                    pRect2->height = 0;
                }
            }
        }

        if (flag) { // add the face to target
            // represent rectangle position by left-top coordinate
            pRect->x -= (pRect->width >> 1);
            pRect->y -= (pRect->width >> 1);

            pRect->height = pRect->width;
            srcFaceSet[tarFaceCount++] = *pRect;
        }
    }

    return tarFaceCount;
}

int OD_CreateAntiVariationRT(TAntiVarRT *pRT, const int maxObjectNum)
{
    pRT->objs = 0;
    pRT->maxObjs = maxObjectNum;
    pRT->ctx = (TAntiVarCtx*)vmalloc(maxObjectNum * sizeof(TAntiVarCtx));
    if (!pRT->ctx)
        goto err_exit;
    pRT->distMat = (int*)vmalloc(maxObjectNum * maxObjectNum * sizeof(int));
    if (!pRT->distMat)
        goto err_exit;
    pRT->detectedOrphan = (int*)vmalloc(maxObjectNum * sizeof(int));
    if (!pRT->detectedOrphan)
        goto err_exit;
    pRT->loggedOrphan = (int*)vmalloc(maxObjectNum * sizeof(int));
    if (!pRT->loggedOrphan)
        goto err_exit;
    pRT->rmIdx = (int*)vmalloc(maxObjectNum * sizeof(int));
    if (!pRT->rmIdx)
        goto err_exit;

    return 1;

err_exit:

    if (pRT->ctx)
        vfree(pRT->ctx);
    if (pRT->distMat)
        vfree(pRT->distMat);
    if (pRT->detectedOrphan)
        vfree(pRT->detectedOrphan);
    if (pRT->loggedOrphan)
        vfree(pRT->loggedOrphan);
    if (pRT->rmIdx)
        vfree(pRT->rmIdx);

    return -1;
}

void OD_AntiVariation(TRect *objectRects, int *objectNum, TAntiVarParam *pParam, TAntiVarRT *pRT)
{
#define MAX_INT  (0x1111111)
    int i;
    int row;
    int col;
    int m = *objectNum;
    int n = pRT->objs;
    int dx;
    int dy;
    int thMove;
    int minDist;
    int minRow;
    int minCol;
    int nDetectedOrphan;
    int nLoggedOrphan;
    int *detectedOrphan = pRT->detectedOrphan;
    int *loggedOrphan = pRT->loggedOrphan;
    int *rmIdx = pRT->rmIdx;
    int rmNum;
    int srcIdx;
    int dstIdx;
    int endIdx;
    TAntiVarCtx *pCtx;
    TRect rect;
    TRect *pRect;

    /* calculate distance matrix */
    for (row = 0; row < m; ++row) {
        // represent rectangle position by its center
        rect.x = objectRects[row].x + (objectRects[row].width >> 1);
        rect.y = objectRects[row].y + (objectRects[row].width >> 1);
        for (col = 0; col < n; ++col) {
            pCtx = &pRT->ctx[col];
            dx = rect.x - pCtx->lastCenter.x;
            dy = rect.y - pCtx->lastCenter.y;
            pRT->distMat[row * pRT->maxObjs + col] = dx * dx + dy * dy;
        }
    }

    for (row = 0; row < m; ++row)
        detectedOrphan[row] = 1;
    for (col = 0; col < n; ++col)
        loggedOrphan[col] = 1;

    /* stabilize object rectangles */
    nDetectedOrphan = m;
    nLoggedOrphan = n;
    for (i = 0; i < min(m, n); ++i) {
        // find (detected, logged) pair with smallest distance
        minDist = MAX_INT;
        minRow = 0;
        minCol = 0;
        for (row = 0; row < m; ++row) {
            for (col = 0; col < n; ++col) {
                if (minDist > pRT->distMat[row * pRT->maxObjs + col]) {
                    minDist = pRT->distMat[row * pRT->maxObjs + col];
                    minRow = row;
                    minCol = col;
                }
            }
        }
        thMove = pRT->ctx[minCol].lastStbWidth >> 1;//(pRT->ctx[minCol].lastStbWidth * pParam->thMoveRatio) >> (7 - 2);
        thMove *= thMove;
        if (minDist < thMove) { // prevent matching of detected- and logged-object orphans
            for (col = 0; col < n; ++col)
                pRT->distMat[minRow * pRT->maxObjs + col] = MAX_INT;
            for (row = 0; row < m; ++row)
                pRT->distMat[row * pRT->maxObjs + minCol] = MAX_INT;
            pRT->ctx[minCol].missDetectCount = 0;
            detectedOrphan[minRow] = 0;
            loggedOrphan[minCol] = 0;
            --nDetectedOrphan;
            --nLoggedOrphan;
#ifdef _DEBUG_
            printk("detected->logged: %d(%d, %d, %d), %d(%d, %d, %d)\n", minRow, objectRects[minRow].x + (objectRects[minRow].width >> 1), objectRects[minRow].y + (objectRects[minRow].width >> 1), objectRects[minRow].width, minCol, pRT->ctx[minCol].lastCenter.x, pRT->ctx[minCol].lastCenter.y, pRT->ctx[minCol].lastStbWidth);
#endif // _DEBUG_
            // stabilize it
            stabilizeOneObject(&objectRects[minRow], pParam, &pRT->ctx[minCol]);
        }
#ifdef _DEBUG_
        else {
            printk("ORPHAN. %d (dist) > %d (thMove)\n", minDist, thMove);
            printk("  detected><logged: %d(%d, %d, %d), %d(%d, %d, %d)\n", minRow, objectRects[minRow].x + (objectRects[minRow].width >> 1), objectRects[minRow].y + (objectRects[minRow].width >> 1), objectRects[minRow].width, minCol, pRT->ctx[minCol].lastCenter.x, pRT->ctx[minCol].lastCenter.y, pRT->ctx[minCol].lastStbWidth);
        }
#endif // _DEBUG_
    }

    if (nDetectedOrphan > 0) {
        /* detected-object orphans */
        for (i = 0; i < m; ++i) {
            if ((detectedOrphan[i] > 0) && (pRT->objs < pRT->maxObjs)) {
                // add new object context
                pRect = &objectRects[i];
                pCtx = &pRT->ctx[pRT->objs];
                pCtx->count = 0;
                pCtx->lastCenter.x = pRect->x + (pRect->width >> 1);
                pCtx->lastCenter.y = pRect->y + (pRect->width >> 1);
                pCtx->missDetectCount = 0;
                pCtx->abnormalMoveCount = 0;
                pCtx->medianWinSize = pParam->maxMedianWinSize;
                pCtx->widthChangeCount = 0;
                pCtx->widthChangeDir = 0;
                pCtx->lastStbWidth = pRect->width;
                ++pRT->objs;
#ifdef _DEBUG_
                printk("add logged: %d(%d, %d, %d)\n", pRT->objs - 1, pCtx->lastCenter.x, pCtx->lastCenter.y, pCtx->lastStbWidth);
#endif // _DEBUG_
            }
        }
    }

    if (nLoggedOrphan > 0) {
        /* logged-object orphans */
        rmNum = 0;
        for (i = 0; i < n; ++i) {
            if (loggedOrphan[i] > 0) {
                pCtx = &pRT->ctx[i];
                if ((pCtx->missDetectCount < pParam->thMissDetectCount) && (*objectNum < pRT->maxObjs)) {
                    // keep it
#ifndef _AV_OUTPUT_LOGGED_ORDER_
                    objectRects[*objectNum].x = pCtx->lastCenter.x - (pCtx->lastStbWidth >> 1);
                    objectRects[*objectNum].y = pCtx->lastCenter.y - (pCtx->lastStbWidth >> 1);
                    objectRects[*objectNum].width = pCtx->lastStbWidth;
                    objectRects[*objectNum].height = pCtx->lastStbWidth;
#endif
#ifdef _DEBUG_
                    printk("add detected: %d(%d, %d, %d)\n", *objectNum, pCtx->lastCenter.x, pCtx->lastCenter.y, pCtx->lastStbWidth);
#endif // _DEBUG_
#ifndef _AV_OUTPUT_LOGGED_ORDER_
                    ++(*objectNum);
#endif
                    ++(pCtx->missDetectCount);
                }
                else {
                    // object disappearance is confirmed, reset the anti-variation job
                    rmIdx[rmNum++] = i;
                }
            }
        }
        if (rmNum > 0) {
            // remove disappeared object
            dstIdx = rmIdx[0];
            for (i = 0; i < rmNum; ++i) {
                pCtx = &pRT->ctx[rmIdx[i]];
#ifdef _DEBUG_
                printk("remove logged: %d(%d, %d, %d)\n", rmIdx[i], pCtx->lastCenter.x, pCtx->lastCenter.y, pCtx->lastStbWidth);
#endif // _DEBUG_
                if (i + 1 < rmNum)
                    endIdx = rmIdx[i + 1];
                else
                    endIdx = pRT->objs;
                for (srcIdx = rmIdx[i] + 1; srcIdx < endIdx; ++srcIdx) {
                    pRT->ctx[dstIdx] = pRT->ctx[srcIdx];
                    ++dstIdx;
                }
            }
            pRT->objs -= rmNum;
        }
    }

#ifdef _AV_OUTPUT_LOGGED_ORDER_
    // copy logged to detected
    for (i = 0; i < pRT->objs; ++i) {
        pCtx = &pRT->ctx[i];
        objectRects[i].x = pCtx->lastCenter.x - (pCtx->lastStbWidth >> 1);
        objectRects[i].y = pCtx->lastCenter.y - (pCtx->lastStbWidth >> 1);
        objectRects[i].width = pCtx->lastStbWidth;
        objectRects[i].height = pCtx->lastStbWidth;
    }
    *objectNum = pRT->objs;
#endif

#ifdef _DEBUG_
    printk("\n");
#endif // _DEBUG_
}

void OD_DestroyAntiVariationRT(TAntiVarRT *pRT)
{
    vfree(pRT->ctx);
    vfree(pRT->distMat);
    vfree(pRT->detectedOrphan);
    vfree(pRT->loggedOrphan);
    vfree(pRT->rmIdx);
}

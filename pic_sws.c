/*
 * pic_sws.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libswscale/swscale.h"
#include "pic_sws.h"


PIC_SWS_TH *alloc_pic_sws_th(int32_t src_w, int32_t src_h, PICTURE_FORMAT src_pic_fmt,
                             int32_t dst_w, int32_t dst_h, PICTURE_FORMAT dst_pic_fmt)
{
    PIC_SWS_TH *psws_th = (PIC_SWS_TH *)malloc(sizeof(PIC_SWS_TH));
    if (NULL == psws_th)
    {
        elog("alloc memery is fail in alloc_pic_sws_th.\n");
        return NULL;
    }


    psws_th->sws_ctx = sws_getContext(src_w, src_h, libv_get_ffm_fmt_by_pic_fmt(src_pic_fmt),
                                      dst_w, dst_h, libv_get_ffm_fmt_by_pic_fmt(dst_pic_fmt),
                                      SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (psws_th->sws_ctx == NULL)
    {
        elog("Alloc sws_getContext is error!");
        free(psws_th);
        return NULL;
    }
    return psws_th;
}
void free_pic_sws_th(PIC_SWS_TH *psws_th)
{
    if (NULL != psws_th)
    {
        sws_freeContext(psws_th->sws_ctx);
        free(psws_th);
    }
}

//返回输出的高度。
int32_t pic_sws_scale(PIC_SWS_TH *psws_th, FRAME_PIC_DATA *psrc_pic, FRAME_PIC_DATA *pdst_pic)
{
    if (NULL == psws_th)
    {
        elog("input is error!");
        return -1;
    }

    return sws_scale(psws_th->sws_ctx, (const uint8_t * const*)psrc_pic->data,
               psrc_pic->linesize, 0, psrc_pic->height, pdst_pic->data, pdst_pic->linesize);
}


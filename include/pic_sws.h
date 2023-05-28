/*
 * pic_sws.h
 *  图片格式转换，包括图片格式，以及分辨率。
 */

#ifndef YUV_SWS_H_
#define YUV_SWS_H_

#include <stdint.h>
#include "libv_log.h"
#include "libvideo.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct __PIC_SWS_TH
{
    struct SwsContext *sws_ctx;     //ffm的分辨率转换句柄
}PIC_SWS_TH;

PIC_SWS_TH *alloc_pic_sws_th(int32_t src_w, int32_t src_h, PICTURE_FORMAT src_pic_fmt,
                             int32_t dst_w, int32_t dst_h, PICTURE_FORMAT dst_pic_fmt);
void free_pic_sws_th(PIC_SWS_TH *psws_th);

//返回输出的高度。pdst_pic需要上层申请足够的内存。
int32_t pic_sws_scale(PIC_SWS_TH *psws_th, FRAME_PIC_DATA *psrc_pic, FRAME_PIC_DATA *pdst_pic);








#ifdef __cplusplus
}
#endif


#endif /* YUV_SWS_H_ */

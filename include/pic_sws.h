/*
 * pic_sws.h
 *  ͼƬ��ʽת��������ͼƬ��ʽ���Լ��ֱ��ʡ�
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
    struct SwsContext *sws_ctx;     //ffm�ķֱ���ת�����
}PIC_SWS_TH;

PIC_SWS_TH *alloc_pic_sws_th(int32_t src_w, int32_t src_h, PICTURE_FORMAT src_pic_fmt,
                             int32_t dst_w, int32_t dst_h, PICTURE_FORMAT dst_pic_fmt);
void free_pic_sws_th(PIC_SWS_TH *psws_th);

//��������ĸ߶ȡ�pdst_pic��Ҫ�ϲ������㹻���ڴ档
int32_t pic_sws_scale(PIC_SWS_TH *psws_th, FRAME_PIC_DATA *psrc_pic, FRAME_PIC_DATA *pdst_pic);








#ifdef __cplusplus
}
#endif


#endif /* YUV_SWS_H_ */

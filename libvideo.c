/*
 * libvideo.c
 *
 */

#include "libvideo.h"
#include "h264_decoder.h"
#include "h264_encoder.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavutil/avutil.h"

static uint8_t version_str[256] = "";
#define VERSION "libvideo.so_R1.0.0"

//��ffmpeg��ö��AVPictureType ת��ΪPICTURE_TYPE
PICTURE_TYPE libv_get_pic_type_by_ffm_type(enum AVPictureType type)
{
    switch(type)
    {
        case AV_PICTURE_TYPE_I:
            return PICTURE_TYPE_I;
        case AV_PICTURE_TYPE_P:
            return PICTURE_TYPE_P;
        case AV_PICTURE_TYPE_B:
            return PICTURE_TYPE_B;
        case AV_PICTURE_TYPE_S:
            return PICTURE_TYPE_S;
        case AV_PICTURE_TYPE_SI:
            return PICTURE_TYPE_SI;
        case AV_PICTURE_TYPE_SP:
            return PICTURE_TYPE_SP;
        case AV_PICTURE_TYPE_BI:
            return PICTURE_TYPE_BI;
        default:
            return PICTURE_TYPE_NONE;
    }
}
//��ffmpeg��ö��AVPictureType ת��ΪPICTURE_TYPE
PICTURE_FORMAT libv_get_pic_fmt_by_ffm_fmt(int32_t fmt)
{
    switch(fmt)
    {
        case AV_PIX_FMT_YUV420P:
            return PIC_FMT_YUV_I420;
        default:
            return PIC_FMT_NONE;
    }
}
int32_t libv_get_ffm_fmt_by_pic_fmt(PICTURE_FORMAT fmt)
{
    switch(fmt)
    {
        case PIC_FMT_YUV_I420:
            return AV_PIX_FMT_YUV420P;
        default:
            return AV_PIX_FMT_NONE;
    }
}

void libv_init(LIBV_LOG_FUN log_cbfun, LIBV_LOG_LEVEL_E level, const char* log_file, int printf_flg)
{
    snprintf((char *)version_str, sizeof(version_str), "\t%s | %s %s\n", VERSION, __DATE__, __TIME__);

    //��־��ʼ��
    libv_log_set(log_cbfun, level, log_file, printf_flg);

#ifndef VIDEO_NO_ENC_DEC
    ilog("now libviedo has encoder and decoder.");
    h264_decoder_init();
    h264_encoder_init();
#endif

}
uint8_t * libv_get_version()
{
#ifndef VIDEO_NO_ENC_DEC
    print_openh264_version();
#endif
    return version_str;
}

//����һ���µ�pictures�������ڲ���������data���ڴ棬��Ҫ�ϲ��Լ����䡣��Ҫ����libvideo_free_frame_pic�ͷš�
FRAME_PIC_DATA *libv_alloc_frame_pic(PICTURE_FORMAT pic_format, int32_t width, int32_t height)
{
    FRAME_PIC_DATA * frame_pic = NULL;
    int32_t i = 0;

    if (pic_format != PIC_FMT_YUV_I420)
    {
        elog("libvideo_alloc_frame_pic:pic_format is not PIC_FMT_YUV_I420!");
        return NULL;
    }

    frame_pic = (FRAME_PIC_DATA *)malloc(sizeof(FRAME_PIC_DATA));
    if (frame_pic == NULL)
    {
        elog("libvideo_alloc_frame_pic:alloc memery is fail.");
        return NULL;
    }

    frame_pic->pic_format = pic_format;
    frame_pic->width = width;
    frame_pic->height = height;
    frame_pic->_alloc_flg = 0;

    for (i = 0; i < LIBV_NUM_DATA_POINTERS; i++)    //���ﲻΪ���������ڴ档�������ϲ����ɵ�ѡ����ʲô�ڴ档
    {
        frame_pic->data[i] = NULL;
        frame_pic->linesize[i] = 0;
    }

    return frame_pic;
}

//����һ���µ�pictures���������������ڴ棬��psrc_pic�����е����ݶ�������������Ҫ����libvideo_free_frame_pic�ͷš�
FRAME_PIC_DATA *libv_copy_new_frame_pic(FRAME_PIC_DATA *psrc_pic)
{
    FRAME_PIC_DATA * frame_pic = NULL;
    int32_t  y_len = 0;
    int32_t  u_len = 0;
    int32_t  v_len = 0;
    uint8_t *pbuf = NULL;

    if (NULL == psrc_pic)
    {
        elog("libvideo_alloc_frame_pic input is error.");
        return NULL;
    }

    frame_pic = libv_alloc_frame_pic(psrc_pic->pic_format, psrc_pic->width, psrc_pic->height);
    if (frame_pic == NULL)
    {
        elog("libvideo_alloc_frame_pic excute return fail.");
        return NULL;
    }
    memcpy(frame_pic, psrc_pic, sizeof(FRAME_PIC_DATA));
    frame_pic->_alloc_flg = 0;  //�������������ڴ��־Ϊ0����ֹ����

    //��ʼ�����ڴ沢������
    if (psrc_pic->data[0] != NULL)
    {
        y_len = psrc_pic->height * psrc_pic->linesize[0] ;
        u_len = psrc_pic->height / 2 * psrc_pic->linesize[1];
        v_len = psrc_pic->height / 2 * psrc_pic->linesize[2];
        pbuf = (uint8_t *)malloc(y_len + u_len + v_len);//����yuv�ܳ������ﲻ����ʹ�ÿ�ȡ�
        if (pbuf == NULL)
        {
            elog("Alloc memery is fail.");
            libv_free_frame_pic(frame_pic);
            return NULL;
        }
        frame_pic->data[0] = pbuf;
        frame_pic->data[1] = frame_pic->data[0] + y_len;
        frame_pic->data[2] = frame_pic->data[1] + u_len;

        frame_pic->linesize[0] = psrc_pic->linesize[0];
        frame_pic->linesize[1] = psrc_pic->linesize[1];
        frame_pic->linesize[2] = psrc_pic->linesize[2];
        frame_pic->_alloc_flg = 1;

        //��ʼ����
        memcpy(frame_pic->data[0], psrc_pic->data[0], y_len);
        memcpy(frame_pic->data[1], psrc_pic->data[1], u_len);
        memcpy(frame_pic->data[2], psrc_pic->data[2], v_len);
    }

    return frame_pic;
}
//��libvideo_alloc_frame_pic�������ǣ�����Ϊdata�����ڴ档
FRAME_PIC_DATA *libv_alloc_frame_pic_mem(PICTURE_FORMAT pic_format, int32_t width, int32_t height)
{
    FRAME_PIC_DATA * frame_pic = NULL;
    int32_t  y_len = 0;
    int32_t  u_len = 0;
    int32_t  v_len = 0;
    int32_t linesize = 0;
    uint8_t *pbuf = NULL;

    frame_pic = libv_alloc_frame_pic(pic_format, width, height);
    if (frame_pic == NULL)
    {
        elog("libvideo_alloc_frame_pic excute return fail.");
        return NULL;
    }

    linesize = width + 32;  //ÿ�ж��32���ֽڡ����ֱ����ⶼ����������ġ�
    y_len = height * linesize;
    u_len = height * linesize>>2;
    v_len = y_len>>2;
    pbuf = (uint8_t *)malloc(y_len + u_len + v_len);//����yuv�ܳ������ﲻ����ʹ�ÿ�ȡ�
    if (pbuf == NULL)
    {
        elog("Alloc memery is fail.");
        libv_free_frame_pic(frame_pic);
        return NULL;
    }
    memset(pbuf, 0, y_len + u_len + v_len);

    frame_pic->data[0] = pbuf;
    frame_pic->data[1] = frame_pic->data[0] + y_len;
    frame_pic->data[2] = frame_pic->data[1] + u_len;

    frame_pic->linesize[0] = linesize;
    frame_pic->linesize[1] = linesize>>1;
    frame_pic->linesize[2] = linesize>>1;
    frame_pic->_alloc_flg = 1;

    return frame_pic;
}

void libv_free_frame_pic(FRAME_PIC_DATA * frame_pic)
{
    if (frame_pic)
    {
        if (frame_pic->_alloc_flg)
        {
            free(frame_pic->data[0]);
        }
        free(frame_pic);
    }
}









/*
 * openh264_decoder.c
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "codec_def.h"
#include "codec_app_def.h"
#include "codec_api.h"

#include "h264_decoder.h"
#include "libvideo.h"


//缓存的时戳的最大个数
#define  MAX_TIMESTAMP_COUNT   64

typedef struct __TIMESTAMP_ST
{
    int32_t used_flg;
    uint32_t timestamp;
}TIMESTAMP_ST;

//解码库的私有数据结构体
typedef  struct __OPENH264_DEOCDER_ST
{
    ISVCDecoder* pdecoder;
    SDecodingParam sdec_param;

//    TIMESTAMP_ST timestamps[MAX_TIMESTAMP_COUNT];   //环形数据结构
    int32_t  ts_start; //起始下标
    int32_t  ts_end; //结束下标
}OPENH264_DEOCDER_ST;

//库中支持设置时戳，这个删除。
//static void openh264_add_timestamp(OPENH264_DEOCDER_ST *pffm, uint32_t ts)
//{
//    if ((pffm->ts_end >= MAX_TIMESTAMP_COUNT) || (pffm->ts_end < 0))
//    {
//        elog("timestamps index[end:%d] is error!initial all.", pffm->ts_end);
//        pffm->ts_end = 0;
//        pffm->ts_start = 0;
//        memset(pffm->timestamps, 0, sizeof(pffm->timestamps));
//    }
//    if (pffm->timestamps[pffm->ts_end].used_flg == 0)
//    {
//        pffm->timestamps[pffm->ts_end].used_flg = 1;
//        pffm->timestamps[pffm->ts_end].timestamp = ts;
//        pffm->ts_end += 1;
//        if (pffm->ts_end >= MAX_TIMESTAMP_COUNT)
//        {
//            pffm->ts_end = 0;
//        }
//    }
//    else
//    {
//        wlog("openh264_add_timestamp:there is no enough timestamp buf for add.");
//    }
//}
//
//static uint32_t openh264_pop_timestamp(OPENH264_DEOCDER_ST *pffm)
//{
//    uint32_t ts = 0;
//
//    if ((pffm->ts_start >= MAX_TIMESTAMP_COUNT) || (pffm->ts_start < 0))
//    {
//        elog("timestamps index[ts_start:%d] is error!initial all.", pffm->ts_end);
//        pffm->ts_end = 0;
//        pffm->ts_start = 0;
//        memset(pffm->timestamps, 0, sizeof(pffm->timestamps));
//    }
//
//    if (pffm->timestamps[pffm->ts_start].used_flg != 0)
//    {
//        pffm->timestamps[pffm->ts_start].used_flg = 0;
//        ts = pffm->timestamps[pffm->ts_start].timestamp;
//        pffm->timestamps[pffm->ts_start].timestamp = 0;
//        pffm->ts_start += 1;
//        if (pffm->ts_start >= MAX_TIMESTAMP_COUNT)
//        {
//            pffm->ts_start = 0;
//        }
//        return ts;
//    }
//    else
//    {
//        wlog("openh264_add_timestamp:The start timestamp not used.");
//    }
//    return 0;
//}

void print_openh264_version()
{
    OpenH264Version ver = WelsGetCodecVersion();
    printf("openh264 version is :%d.%d.%d.%d.\n", ver.uMajor, ver.uMinor, ver.uReserved, ver.uRevision);
}

static int32_t openh264_decode_video(H264_DECODER_TH *pdecth, FRAME_PIC_DATA *pout_pic,
                                    int32_t *got_picture_ptr, const H264_NAL_DATA *pnal)
{
    unsigned char * pdata[3] = {NULL};
    SBufferInfo dst_bufinfo;
    SSysMEMBuffer *psys_buf = NULL;
    DECODING_STATE ret = dsErrorFree;
    int32_t len = 0;

    if ((NULL == pdecth) || (pout_pic == NULL) || (got_picture_ptr == NULL) || (pnal == NULL))
    {
        elog("openh264_release:input is error!");
        return -1;
    }

    OPENH264_DEOCDER_ST *popenst = (OPENH264_DEOCDER_ST *)pdecth->_private_data;
    if (NULL == popenst)
    {
        elog("H264_DECODER_TH private_data is NULL!");
        return -1;
    }

    pdata[0] = NULL;
    pdata[1] = NULL;
    pdata[2] = NULL;
    memset(&dst_bufinfo, 0, sizeof (dst_bufinfo));
    dst_bufinfo.uiInBsTimeStamp = pnal->timestamp;

//    ret = (*(popenst->pdecoder))->DecodeFrame2 (popenst->pdecoder, pnal->data_buf, pnal->data_size,
//                                           pdata, &dst_bufinfo);
    ret = (*(popenst->pdecoder))->DecodeFrameNoDelay (popenst->pdecoder, pnal->data_buf, pnal->data_size,
                                           pdata, &dst_bufinfo);
    if (ret == 0)
    {
        len = pnal->data_size;
    }
    else
    {
        elog("Error while decoding frame . return[%x] ", ret);//这里不返回，还有看看是否有帧输出
        len = -1;
    }

    if (dst_bufinfo.iBufferStatus == 1)
    {
        *got_picture_ptr = 1;
        psys_buf = &(dst_bufinfo.UsrData.sSystemBuffer);

        if (psys_buf->iFormat == videoFormatI420)   //目前遇到的情况只有i420，如果遇到花屏或其他，可以在这里修改
        {
            pout_pic->pic_format = PIC_FMT_YUV_I420;
        }
        else
        {
            elog("unkonwn picture format:%d", psys_buf->iFormat);
            pout_pic->pic_format = PIC_FMT_NONE;
        }

        pout_pic->pic_type = PICTURE_TYPE_I;
        pout_pic->key_frame = 0;
        pout_pic->quality = 0;
        pout_pic->width = psys_buf->iWidth;
        pout_pic->height = psys_buf->iHeight;
        pout_pic->timestamp = dst_bufinfo.uiOutYuvTimeStamp;  //openh264_pop_timestamp(popenst);

        pout_pic->data[0] = (uint8_t*)pdata[0];
        pout_pic->data[1] = (uint8_t*)pdata[1];
        pout_pic->data[2] = (uint8_t*)pdata[2];

        pout_pic->linesize[0] = psys_buf->iStride[0];
        pout_pic->linesize[1] = psys_buf->iStride[1];
        pout_pic->linesize[2] = psys_buf->iStride[1];
    }
    else
    {
        *got_picture_ptr = 0;
    }
    return len;
}

static void openh264_release(H264_DECODER_TH *pdecth)
{
    if (NULL == pdecth)
    {
        elog("openh264_release:input is error!");
        return ;
    }

    OPENH264_DEOCDER_ST *popendec = (OPENH264_DEOCDER_ST *)pdecth->_private_data;
    if (popendec)
    {
        (*(popendec->pdecoder))->Uninitialize(popendec->pdecoder);
        WelsDestroyDecoder (popendec->pdecoder);
        free(popendec);
    }
}

static int32_t openh264_init(H264_DECODER_TH *pdecth)
{
    if (NULL == pdecth)
    {
        elog("openh264_init:input is error!");
        return -1;
    }
    OPENH264_DEOCDER_ST *popdec = (OPENH264_DEOCDER_ST *)malloc(sizeof(OPENH264_DEOCDER_ST));
    if (NULL == popdec)
    {
        elog("malloc memery is fail!");
        return -1;
    }

    memset(popdec, 0, sizeof(OPENH264_DEOCDER_ST));

    popdec->sdec_param.sVideoProperty.size = sizeof (popdec->sdec_param.sVideoProperty);

    popdec->sdec_param.eOutputColorFormat    = videoFormatI420;
//    popdec->sdec_param.iOutputColorFormat    = videoFormatI420;
    popdec->sdec_param.uiTargetDqLayer   = (uint8_t) - 1;
    popdec->sdec_param.eEcActiveIdc = ERROR_CON_FRAME_COPY;
    popdec->sdec_param.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;

    if (WelsCreateDecoder (&(popdec->pdecoder))  || (NULL == popdec->pdecoder))
    {
        elog("Create openh264 Decoder failed.");
        return -1;
    }

    //设置日志
    CM_RETURN eret = 0;
    WelsTraceCallback callback = openh264_log_callback;
    eret = (*(popdec->pdecoder))->SetOption(popdec->pdecoder, DECODER_OPTION_TRACE_CALLBACK, &callback);

    eret = (*(popdec->pdecoder))->Initialize (popdec->pdecoder, &(popdec->sdec_param));
    if (eret)
    {
        WelsDestroyDecoder (popdec->pdecoder);
        elog("Openh264 Decoder initialization failed. Return is %d\n", eret);
        return -1;
    }

    pdecth->_private_data = (void *)popdec;
    pdecth->decode_video = openh264_decode_video;

    return 0;
}

void openh264_decoder_init()
{
    H264_DECODER_FACTORY *pfact  = NULL;

    pfact = (H264_DECODER_FACTORY *)malloc(sizeof(H264_DECODER_FACTORY));
    if (NULL == pfact)
    {
        elog("openh264_decoder_init:malloc memery is fail!");
        exit(1);
    }
    pfact->id = H264_DECODER_ID_OPENH264;
    pfact->init = openh264_init;
    pfact->release = openh264_release;

    h264_decoder_register(pfact);
}










/*
 * ffm_decoder.c
 *  ffmpeg 的h264解码库
 */

#include "ffm_decoder.h"
#include "h264_decoder.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"

//缓存的时戳的最大个数
#define  MAX_TIMESTAMP_COUNT   64

typedef struct __TIMESTAMP_ST
{
    int32_t used_flg;
    uint32_t timestamp;
}TIMESTAMP_ST;

//解码库的私有数据结构体
typedef  struct __FFM_DEOCDER_ST
{
    AVCodec *h264_codec;
    AVCodecContext *h264_codec_ctx;
    AVFrame *h264_frame;
    AVPacket h264_avpkt;

    struct SwsContext *sws_ctx;     //图片格式转换句柄，保证给上层的数据都是YUV420p
    enum AVPixelFormat src_fmt;     //解码器输出的图片格式，如果不等于AV_PIX_FMT_YUV420P则会进行格式转换。
    int32_t src_width;              //解码器输出的图片的宽度
    int32_t src_height;             //解码器输出的图片的高度
    uint8_t *yuv_buf;               //有图片格式转换时临时使用的缓冲区
    int32_t  buf_len;               //yuv_buf的长度
    uint8_t *sws_data[LIBV_NUM_DATA_POINTERS];      //转换后的临时输出，内存使用yuv_buf
    int32_t sws_linesize[LIBV_NUM_DATA_POINTERS];   //转换后的临时输出

    TIMESTAMP_ST timestamps[MAX_TIMESTAMP_COUNT];   //环形数据结构
    int32_t  ts_start; //起始下标
    int32_t  ts_end; //结束下标
}FFM_DEOCDER_ST;

//用于解码器图片输出格式不为420p的情况下，转化为420p，下面是转换临时缓冲区长度，暂时定为4K。
static const int32_t  G_FMT_SWS_BUF_LEN = 1920 * 1080 * 4 + 32;

static void ffm_release(H264_DECODER_TH *pdecth)
{
    if (NULL == pdecth)
    {
        elog("ffm_release:input is error!");
        return ;
    }

    FFM_DEOCDER_ST *pffm = (FFM_DEOCDER_ST *)pdecth->_private_data;
    if (pffm)
    {
        avcodec_close(pffm->h264_codec_ctx);
        av_free(pffm->h264_codec_ctx);
        av_frame_free(&(pffm->h264_frame));

        if (pffm->sws_ctx != NULL)
        {
            sws_freeContext(pffm->sws_ctx);
        }
        if (pffm->yuv_buf != NULL)
        {
            free(pffm->yuv_buf);
        }

        free(pffm);
    }
}
//这两个时戳处理函数暂时不用
//static void ffm_add_timestamp(FFM_DEOCDER_ST *pffm, uint32_t ts)
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
//        elog("ffmdeocer:there is no enough timestamp buf for add.");
//    }
//}
//
//static uint32_t ffm_pop_timestamp(FFM_DEOCDER_ST *pffm)
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
//        wlog("ffmdeocer:The start timestamp not used.");
//    }
//    return 0;
//}

//将解码器的输出由其他格式转化为yuv420p，成功返回0，失败返回-1，转换后的输出保存在：pffm->sws_data和pffm->sws_linesize
static int32_t ffm_sws_decoder_output_pic_to_yuv420p(FFM_DEOCDER_ST *pffm, AVFrame *h264_frame)
{
    int32_t yuv_len = 0;

    if ((NULL == pffm) || (h264_frame == NULL))
    {
        elog("ffm_sws_decoder_output_pic_to_yuv420p:input is error!");
        return -1;
    }

    if (pffm->sws_ctx != NULL)
    {
        if ((pffm->src_fmt != h264_frame->format) || (pffm->src_height != h264_frame->height)
                || (pffm->src_width != h264_frame->width))//原先申请的转换器和目前的不匹配，重新申请。主要考虑视频流的格式，高宽会改变
        {
            sws_freeContext(pffm->sws_ctx); //先释放。
            pffm->sws_ctx = NULL;
            pffm->sws_ctx = sws_getContext(h264_frame->width, h264_frame->height, h264_frame->format,
                                           h264_frame->width, h264_frame->height, AV_PIX_FMT_YUV420P,
                                           SWS_FAST_BILINEAR, NULL, NULL, NULL);
            if (pffm->sws_ctx == NULL)
            {
                elog("decoder return form is not yuv420p, switch but sws_getContext return NULL!");
                return -1;
            }
        }
    }
    else
    {
        pffm->sws_ctx = sws_getContext(h264_frame->width, h264_frame->height, h264_frame->format,
                                       h264_frame->width, h264_frame->height, AV_PIX_FMT_YUV420P,
                                       SWS_FAST_BILINEAR, NULL, NULL, NULL);
        if (pffm->sws_ctx == NULL)
        {
            elog("decoder return form is not yuv420p, switch but sws_getContext return NULL!");
            return -1;
        }
    }
    //申请成功，则保存参数
    pffm->src_fmt = h264_frame->format;
    pffm->src_height = h264_frame->height;
    pffm->src_width = h264_frame->width;

    //设置yuv 420的数据,首先判断缓冲区释放足够。
    yuv_len = h264_frame->height * h264_frame->width;
    if ((yuv_len * 3 /2) > pffm->buf_len)
    {
        wlog("switch decoder output pic,but buf is not long enough!");
        return -1;
    }
    pffm->sws_data[0] = pffm->yuv_buf;
    pffm->sws_data[1] = pffm->sws_data[0] + yuv_len;
    pffm->sws_data[2] = pffm->sws_data[1] + yuv_len / 4 ;

    pffm->sws_linesize[0] = h264_frame->width;
    pffm->sws_linesize[1] = pffm->sws_linesize[2] = h264_frame->width>>1;

    sws_scale(pffm->sws_ctx, (const uint8_t * const*)h264_frame->data,
              h264_frame->linesize, 0, h264_frame->height, pffm->sws_data, pffm->sws_linesize);
    return 0;
}

static int32_t ffm_decode_video(H264_DECODER_TH *pdecth, FRAME_PIC_DATA *pout_pic,
                            int32_t *got_picture_ptr, const H264_NAL_DATA *pnal)
{
    int32_t len = 0;
    AVFrame *h264_frame = NULL;
    int32_t i = 0;
    if ((NULL == pdecth) || (pout_pic == NULL) || (got_picture_ptr == NULL) || (pnal == NULL))
    {
        elog("ffm_decode_video:input is error!");
        return -1;
    }

    FFM_DEOCDER_ST *pffm = (FFM_DEOCDER_ST *)pdecth->_private_data;
    if (NULL == pffm)
    {
        elog("H264_DECODER_TH private_data is NULL!");
        return -1;
    }

    pffm->h264_avpkt.size = pnal->data_size;
    pffm->h264_avpkt.data = pnal->data_buf;
    pffm->h264_avpkt.pts = pnal->timestamp;
    pffm->h264_avpkt.dts = pnal->timestamp;
    h264_frame = pffm->h264_frame;
    len = avcodec_decode_video2(pffm->h264_codec_ctx, h264_frame, got_picture_ptr, &(pffm->h264_avpkt));
    if (len < 0)
    {
//        elog("Error while decoding frame . len[%d] ", len);//这里不返回，还有看看是否有帧输出:没有帧是经常会输出负值，并打印错误。这里先注释，后面看看最新版本是否解决此问题
    }
    else    //如果没有失败，则记录时戳
    {
//        ffm_add_timestamp(pffm, pnal->timestamp);
    }
    if (*got_picture_ptr)
    {
//        printf("===================return timestamp:%d,%d,%d\n", h264_frame->pts, h264_frame->pkt_pts, h264_frame->pkt_dts);

        pout_pic->pic_type = libv_get_pic_type_by_ffm_type(h264_frame->pict_type);
        pout_pic->key_frame = h264_frame->key_frame;
        pout_pic->quality = h264_frame->quality;
        pout_pic->width = h264_frame->width;
        pout_pic->height = h264_frame->height;
        pout_pic->timestamp = h264_frame->pkt_dts;  //不用ffm_pop_timestamp(pffm);，直接使用解码器输出。

        if (h264_frame->format == AV_PIX_FMT_YUV420P)   //目前遇到的情况只有i420，如果遇到花屏或其他，可以在这里修改
        {
            for (i = 0; (i < LIBV_NUM_DATA_POINTERS) && (i < AV_NUM_DATA_POINTERS); i++)
            {
                pout_pic->data[i] = h264_frame->data[i];
                pout_pic->linesize[i] = h264_frame->linesize[i];
            }
        }
        else
        {
            //如果格式不对，则进行转换：
            if (0 == ffm_sws_decoder_output_pic_to_yuv420p(pffm, h264_frame))
            {
                for (i = 0; (i < LIBV_NUM_DATA_POINTERS); i++)
                {
                    pout_pic->data[i] = pffm->sws_data[i];
                    pout_pic->linesize[i] = pffm->sws_linesize[i];
                }
            }
            else
            {
                wlog("switch deocder output pic from [%d] to yuv420p is error!", h264_frame->format);
                return -1;
            }
        }
        pout_pic->pic_format = PIC_FMT_YUV_I420;
    }
    return len;
}


static int32_t ffm_decode_video2(H264_DECODER_TH *pdecth, FRAME_PIC_DATA *pout_pic,
                                int32_t *got_picture_ptr, const H264_NAL_DATA *pnal)
{
    int32_t len = 0;
    AVFrame *h264_frame = NULL;
    int32_t i = 0;
    int ret = 0;
    if ((NULL == pdecth) || (pout_pic == NULL) || (got_picture_ptr == NULL) || (pnal == NULL))
    {
        elog("ffm_decode_video:input is error!");
        return -1;
    }

    FFM_DEOCDER_ST *pffm = (FFM_DEOCDER_ST *)pdecth->_private_data;
    if (NULL == pffm)
    {
        elog("H264_DECODER_TH private_data is NULL!");
        return -1;
    }

    pffm->h264_avpkt.size = pnal->data_size;
    pffm->h264_avpkt.data = pnal->data_buf;
    pffm->h264_avpkt.pts = pnal->timestamp;
    pffm->h264_avpkt.dts = pnal->timestamp;
    h264_frame = pffm->h264_frame;
    ret = avcodec_send_packet(pffm->h264_codec_ctx, &(pffm->h264_avpkt));

    ret = avcodec_receive_frame(pffm->h264_codec_ctx, h264_frame);
    if(ret>=0){
        *got_picture_ptr = 1;
    }
    if (*got_picture_ptr)
    {
//        printf("===================return timestamp:%d,%d,%d\n", h264_frame->pts, h264_frame->pkt_pts, h264_frame->pkt_dts);

        pout_pic->pic_type = libv_get_pic_type_by_ffm_type(h264_frame->pict_type);
        pout_pic->key_frame = h264_frame->key_frame;
        pout_pic->quality = h264_frame->quality;
        pout_pic->width = h264_frame->width;
        pout_pic->height = h264_frame->height;
        pout_pic->timestamp = h264_frame->pkt_dts;  //不用ffm_pop_timestamp(pffm);，直接使用解码器输出。

        if (h264_frame->format == AV_PIX_FMT_YUV420P)   //目前遇到的情况只有i420，如果遇到花屏或其他，可以在这里修改
        {
            for (i = 0; (i < LIBV_NUM_DATA_POINTERS) && (i < AV_NUM_DATA_POINTERS); i++)
            {
                pout_pic->data[i] = h264_frame->data[i];
                pout_pic->linesize[i] = h264_frame->linesize[i];
            }
        }
        else
        {
            //如果格式不对，则进行转换：
            if (0 == ffm_sws_decoder_output_pic_to_yuv420p(pffm, h264_frame))
            {
                for (i = 0; (i < LIBV_NUM_DATA_POINTERS); i++)
                {
                    pout_pic->data[i] = pffm->sws_data[i];
                    pout_pic->linesize[i] = pffm->sws_linesize[i];
                }
            }
            else
            {
                wlog("switch deocder output pic from [%d] to yuv420p is error!", h264_frame->format);
                return -1;
            }
        }
        pout_pic->pic_format = PIC_FMT_YUV_I420;
    }
    return len;
}

static int32_t ffm_init(H264_DECODER_TH *pdecth)
{
    if (NULL == pdecth)
    {
        elog("ffm_init:input is error!");
        return -1;
    }
    FFM_DEOCDER_ST *pffm = (FFM_DEOCDER_ST *)malloc(sizeof(FFM_DEOCDER_ST));
    if (NULL == pffm)
    {
        elog("malloc memery is fail!");
        return -1;
    }

    memset(pffm, 0, sizeof(FFM_DEOCDER_ST));

    av_init_packet(&(pffm->h264_avpkt));

    pffm->h264_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (pffm->h264_codec == NULL)
    {
        elog("avcodec_find_decoder error!");
        free(pffm);
        return -1;
    }

    pffm->h264_codec_ctx = avcodec_alloc_context3(pffm->h264_codec);
    if (pffm->h264_codec_ctx == NULL)
    {
        elog("avcodec_alloc_context3 error!\n");
        free(pffm);
        return -1;
    }

    int ret = avcodec_open2(pffm->h264_codec_ctx, pffm->h264_codec, NULL);

    if (ret < 0)
    {
        elog("Could not open codec.%s",av_err2str(ret));
        av_free(pffm->h264_codec_ctx);
        free(pffm);
        return -1;
    }

    pffm->h264_frame = av_frame_alloc();
    if (pffm->h264_frame == NULL)
    {
        elog("Could not allocate video frame");
        avcodec_close(pffm->h264_codec_ctx);
        av_free(pffm->h264_codec_ctx);
        free(pffm);
        return -1;
    }

    pffm->sws_ctx = NULL;
    pffm->src_fmt = AV_PIX_FMT_YUV420P;
    pffm->src_width = 0;
    pffm->src_height = 0;
    pffm->buf_len = G_FMT_SWS_BUF_LEN;
    pffm->yuv_buf = (uint8_t *)malloc(G_FMT_SWS_BUF_LEN);
    if (NULL == pffm->yuv_buf)
    {
        elog("malloc yuv_buf MEMERY is fail: len is %d\n", G_FMT_SWS_BUF_LEN);
        avcodec_close(pffm->h264_codec_ctx);
        av_free(pffm->h264_codec_ctx);
        av_frame_free(&(pffm->h264_frame));
        free(pffm);
        return -1;
    }

    pdecth->_private_data = (void *)pffm;
    pdecth->decode_video = ffm_decode_video2;

    return 0;
}

void ffm_decoder_init()
{
    H264_DECODER_FACTORY *pfact  = NULL;

    //ffmpeg初始化函数
    av_log_set_callback(ffm_av_log_callback);

//    av_register_all();
//    avcodec_register_all();

    pfact = (H264_DECODER_FACTORY *)malloc(sizeof(H264_DECODER_FACTORY));
    if (NULL == pfact)
    {
        elog("ffm_decoder_init:malloc memery is fail!");
        exit(1);
    }
    pfact->id = H264_DECODER_ID_FFM;
    pfact->init = ffm_init;
    pfact->release = ffm_release;

    h264_decoder_register(pfact);
}




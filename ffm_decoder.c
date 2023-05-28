/*
 * ffm_decoder.c
 *  ffmpeg ��h264�����
 */

#include "ffm_decoder.h"
#include "h264_decoder.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"

//�����ʱ����������
#define  MAX_TIMESTAMP_COUNT   64

typedef struct __TIMESTAMP_ST
{
    int32_t used_flg;
    uint32_t timestamp;
}TIMESTAMP_ST;

//������˽�����ݽṹ��
typedef  struct __FFM_DEOCDER_ST
{
    AVCodec *h264_codec;
    AVCodecContext *h264_codec_ctx;
    AVFrame *h264_frame;
    AVPacket h264_avpkt;

    struct SwsContext *sws_ctx;     //ͼƬ��ʽת���������֤���ϲ�����ݶ���YUV420p
    enum AVPixelFormat src_fmt;     //�����������ͼƬ��ʽ�����������AV_PIX_FMT_YUV420P�����и�ʽת����
    int32_t src_width;              //�����������ͼƬ�Ŀ��
    int32_t src_height;             //�����������ͼƬ�ĸ߶�
    uint8_t *yuv_buf;               //��ͼƬ��ʽת��ʱ��ʱʹ�õĻ�����
    int32_t  buf_len;               //yuv_buf�ĳ���
    uint8_t *sws_data[LIBV_NUM_DATA_POINTERS];      //ת�������ʱ������ڴ�ʹ��yuv_buf
    int32_t sws_linesize[LIBV_NUM_DATA_POINTERS];   //ת�������ʱ���

    TIMESTAMP_ST timestamps[MAX_TIMESTAMP_COUNT];   //�������ݽṹ
    int32_t  ts_start; //��ʼ�±�
    int32_t  ts_end; //�����±�
}FFM_DEOCDER_ST;

//���ڽ�����ͼƬ�����ʽ��Ϊ420p������£�ת��Ϊ420p��������ת����ʱ���������ȣ���ʱ��Ϊ4K��
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
//������ʱ����������ʱ����
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

//���������������������ʽת��Ϊyuv420p���ɹ�����0��ʧ�ܷ���-1��ת�������������ڣ�pffm->sws_data��pffm->sws_linesize
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
                || (pffm->src_width != h264_frame->width))//ԭ�������ת������Ŀǰ�Ĳ�ƥ�䣬�������롣��Ҫ������Ƶ���ĸ�ʽ���߿��ı�
        {
            sws_freeContext(pffm->sws_ctx); //���ͷš�
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
    //����ɹ����򱣴����
    pffm->src_fmt = h264_frame->format;
    pffm->src_height = h264_frame->height;
    pffm->src_width = h264_frame->width;

    //����yuv 420������,�����жϻ������ͷ��㹻��
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
//        elog("Error while decoding frame . len[%d] ", len);//���ﲻ���أ����п����Ƿ���֡���:û��֡�Ǿ����������ֵ������ӡ����������ע�ͣ����濴�����°汾�Ƿ���������
    }
    else    //���û��ʧ�ܣ����¼ʱ��
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
        pout_pic->timestamp = h264_frame->pkt_dts;  //����ffm_pop_timestamp(pffm);��ֱ��ʹ�ý����������

        if (h264_frame->format == AV_PIX_FMT_YUV420P)   //Ŀǰ���������ֻ��i420��������������������������������޸�
        {
            for (i = 0; (i < LIBV_NUM_DATA_POINTERS) && (i < AV_NUM_DATA_POINTERS); i++)
            {
                pout_pic->data[i] = h264_frame->data[i];
                pout_pic->linesize[i] = h264_frame->linesize[i];
            }
        }
        else
        {
            //�����ʽ���ԣ������ת����
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
        pout_pic->timestamp = h264_frame->pkt_dts;  //����ffm_pop_timestamp(pffm);��ֱ��ʹ�ý����������

        if (h264_frame->format == AV_PIX_FMT_YUV420P)   //Ŀǰ���������ֻ��i420��������������������������������޸�
        {
            for (i = 0; (i < LIBV_NUM_DATA_POINTERS) && (i < AV_NUM_DATA_POINTERS); i++)
            {
                pout_pic->data[i] = h264_frame->data[i];
                pout_pic->linesize[i] = h264_frame->linesize[i];
            }
        }
        else
        {
            //�����ʽ���ԣ������ת����
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

    //ffmpeg��ʼ������
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




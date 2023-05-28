/*
 * openh264_encoder.c
 *  ��openh264�����ķ�װ
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "codec_def.h"
#include "codec_app_def.h"
#include "codec_api.h"
#include "ffm_decoder.h"
#include "h264_encoder.h"
#include "h264_ps.h"

#define  MAX_SPS_BUF_LEN    256


//������˽�����ݽṹ��
typedef  struct __OPENH264_ENOCDER_ST
{
    ISVCEncoder *pencoder;
    uint8_t      sps_buf[MAX_SPS_BUF_LEN];  //��ΪҪΪsps���vui��Ϣ�����ԣ����ﱣ��һ����ʱ��������
}OPENH264_ENOCDER_ST;

static FRAME_TYPE get_frame_type_by_openh264_type(EVideoFrameType type)
{
    switch(type)
    {
        case videoFrameTypeIDR:
            return FRAME_TYPE_IDR;
        case videoFrameTypeI:
            return FRAME_TYPE_I;
        case videoFrameTypeP:
            return FRAME_TYPE_P;
        case videoFrameTypeSkip:
            return FRAME_TYPE_SKIP;
        case videoFrameTypeIPMixed:
            return FRAME_TYPE_I_P_MIXED;
        default:
            return FRAME_TYPE_INVALID;
    }
}

static int32_t add_vui_for_sps(H264_ENCODER_TH *pencth, uint8_t *psps_nal, int32_t sps_len)
{
    int32_t  start_len = 0;
    OPENH264_ENOCDER_ST *popenenc = (OPENH264_ENOCDER_ST *)(pencth->_private_data);

    if (pencth->param.frame_rate == 0)
    {
        return -1;  //  ��֪��֡�ʣ�ֱ�ӷ��أ�������ԭ���ġ�
    }

    if (psps_nal[2] == 0)
    {
        start_len = 4;
    }
    else
    {
        start_len = 3;
    }
    memset(popenenc->sps_buf, 0, sizeof(popenenc->sps_buf));
    memcpy(popenenc->sps_buf, psps_nal, start_len + 1); //������ʼ���Լ�nalͷ

    sps_t sps;
    memset(&sps, 0, sizeof(sps));

    dec_seq_parameter_set_rbsp(&sps, psps_nal + start_len + 1, sps_len - start_len - 1);

    sps.vui_parameters_present_flag = 1;
    sps.vui.timing_info_present_flag = 1;
    sps.vui.num_units_in_tick = 1;
    sps.vui.time_scale = pencth->param.frame_rate * 2;

    int32_t out_sps_len = enc_seq_parameter_set_rbsp(&sps, popenenc->sps_buf + start_len + 1,
                                                sizeof(popenenc->sps_buf) - start_len - 1);

    return out_sps_len + start_len + 1;
}

static int32_t openh264_encode_frame(H264_ENCODER_TH *pencth, FRAME_PIC_DATA *pin_pic, LAYER_BS_INFO *player_bsi_out)
{
    OPENH264_ENOCDER_ST *popenenc = NULL;
    SFrameBSInfo sfbi;
    SSourcePicture src_pic ;
    SSourcePicture* psrc_pic = &src_pic;

    if ((NULL == pencth) || (NULL == player_bsi_out) || (NULL == pin_pic))
    {
        elog("openh264_encode_frame:input error!");
        return -1;
    }

    memset(&src_pic, 0, sizeof(src_pic));
    memset(&sfbi, 0, sizeof(sfbi));

    popenenc = (OPENH264_ENOCDER_ST *)(pencth->_private_data);

    psrc_pic->iColorFormat = videoFormatI420;
    psrc_pic->iPicWidth = pin_pic->width;
    psrc_pic->iPicHeight = pin_pic->height;
    psrc_pic->uiTimeStamp = pin_pic->timestamp;
    //update pSrcPic
    psrc_pic->iStride[0] = pin_pic->linesize[0];
    psrc_pic->iStride[1] = pin_pic->linesize[1];
    psrc_pic->iStride[2] = pin_pic->linesize[2];

    psrc_pic->pData[0] = pin_pic->data[0];
    psrc_pic->pData[1] = pin_pic->data[1];
    psrc_pic->pData[2] = pin_pic->data[2];

    int32_t ienc_frames = (*(popenenc->pencoder))->EncodeFrame (popenenc->pencoder, psrc_pic, &sfbi);
    int32_t i = 0;
    unsigned char*  pbuf = NULL;

    if (ienc_frames == cmResultSuccess && videoFrameTypeSkip != sfbi.eFrameType)
    {
        int32_t ilayer = 0;
        H264_NAL_DATA  *pnal = NULL;
        int32_t nal_idx = 0;

//        printf("===========sfbi.iLayerNum[%d] frame type[%d]=======================\n",
//                sfbi.iLayerNum, sfbi.eFrameType);
        player_bsi_out->tmp_id = sfbi.iTemporalId;
        player_bsi_out->frame_type = get_frame_type_by_openh264_type(sfbi.eFrameType);
        player_bsi_out->nal_count = 0;
        while (ilayer < sfbi.iLayerNum)
        {
            SLayerBSInfo* player_bsinfo = &sfbi.sLayerInfo[ilayer];
            if (player_bsinfo != NULL)
            {
                pbuf = player_bsinfo->pBsBuf;
//                printf("===========player_bsinfo->iNalCount[%d]=======================\n",player_bsinfo->iNalCount);
                if (nal_idx >= MAX_NAL_UNITS_COUNT_IN_LAYER)
                {
                    elog("openh264 encoder return nal count is more than %d.", MAX_NAL_UNITS_COUNT_IN_LAYER);
                    return -1;//�����Ĳ��ٻ�ȡ��
                }
                for (i = 0; i < player_bsinfo->iNalCount; i++)
                {
                    pnal = &(player_bsi_out->nal_data[nal_idx]);
                    nal_idx++;  //  �±��һ
                    player_bsi_out->nal_count++;

                    pnal->timestamp = sfbi.uiTimeStamp;

                    if (pbuf[2] == 0)   //�ĸ��ֽڿ�ʼ��
                    {
                        pnal->nal_type = pbuf[4] & 0x1f;
                    }
                    else    //�����ֽڿ�ʼ��
                    {
                        pnal->nal_type = pbuf[3] & 0x1f;
                    }
                    pnal->frame_type = player_bsi_out->frame_type;
                    if (pnal->nal_type == NALU_TYPE_SPS)
                    {
                        pnal->data_size = add_vui_for_sps(pencth, pbuf, player_bsinfo->pNalLengthInByte[i]);
                        if (1)//��ʱɾ����(pnal->data_size < 4 || (pnal->data_size > sizeof(popenenc->sps_buf)))
                        {   //ʧ�ܣ�����ԭ����
                            pnal->data_buf = pbuf;
                            pnal->data_size = player_bsinfo->pNalLengthInByte[i];
                        }
                        else
                        {
                            pnal->data_buf = popenenc->sps_buf;
                        }
                    }
                    else
                    {
                        pnal->data_buf = pbuf;
                        pnal->data_size = player_bsinfo->pNalLengthInByte[i];
                    }
//                    printf("===========pnal->data_size[%d] type[%d]=======================\n",
//                            pnal->data_size, pnal->nal_type);
                    //�����sps����pps������֡������־�������ǻ����mark��־��
                    if ((pnal->nal_type == NALU_TYPE_SPS)|| (pnal->nal_type == NALU_TYPE_PPS))
                    {
                        pnal->frame_end_flg = 1;
                    }
                    else
                    {
                        pnal->frame_end_flg = 0;
                    }

                    pbuf += player_bsinfo->pNalLengthInByte[i];
                }
            }
            ++ ilayer;
        }
        pnal->frame_end_flg = 1;    //���һ��slice�����ý�����־��
    }
    else
    {
        dlog("EncodeFrame error: return [%d]!", ienc_frames);
        return -1;
    }
    return 0;
}

//��ȡsps��pps���ݡ�SPS�ն�֡��openh264Ĭ���ǲ�Я���ġ���Ҫ�Լ���ӡ����Ǳ�����ʺ�ʵ�ʵ�֡����ʱ�򲢲�֪����
static int32_t openh264_encode_parameter_sets(H264_ENCODER_TH *pencth, LAYER_BS_INFO *player_bsi_out)
{
    OPENH264_ENOCDER_ST *popenenc = NULL;
    SFrameBSInfo sfbi;

    if ((NULL == pencth) || (NULL == player_bsi_out))
    {
        elog("openh264_encode_parameter_sets:input error!");
        return -1;
    }

    popenenc = (OPENH264_ENOCDER_ST *)(pencth->_private_data);

    //��ȡsps��pps
    int32_t ienc_frames = (*(popenenc->pencoder))->EncodeParameterSets (popenenc->pencoder, &sfbi);
    int32_t i = 0;
    unsigned char*  pbuf = NULL;

    if (ienc_frames == cmResultSuccess && videoFrameTypeSkip != sfbi.eFrameType)
    {
        int32_t ilayer = 0;
        H264_NAL_DATA  *pnal = NULL;
        int32_t nal_idx = 0;

        player_bsi_out->tmp_id = sfbi.iTemporalId;
        player_bsi_out->frame_type = get_frame_type_by_openh264_type(sfbi.eFrameType);
        player_bsi_out->nal_count = 0;
        while (ilayer < sfbi.iLayerNum)
        {
            SLayerBSInfo* player_bsinfo = &sfbi.sLayerInfo[ilayer];
            if (player_bsinfo != NULL)
            {
                pbuf = player_bsinfo->pBsBuf;
                if (nal_idx >= MAX_NAL_UNITS_COUNT_IN_LAYER)
                {
                    elog("openh264 encoder return nal count is more than %d.", MAX_NAL_UNITS_COUNT_IN_LAYER);
                    return -1;//�����Ĳ��ٻ�ȡ��
                }
                for (i = 0; i < player_bsinfo->iNalCount; i++)
                {
                    pnal = &(player_bsi_out->nal_data[nal_idx]);
                    nal_idx++;
                    player_bsi_out->nal_count++;

                    if (pbuf[2] == 0)   //�ĸ��ֽڿ�ʼ��
                    {
                        pnal->nal_type = pbuf[4] & 0x1f;
                    }
                    else    //�����ֽڿ�ʼ��
                    {
                        pnal->nal_type = pbuf[3] & 0x1f;
                    }
                    pnal->frame_type = player_bsi_out->frame_type;
                    if (pnal->nal_type == NALU_TYPE_SPS)
                    {
                        pnal->data_size = add_vui_for_sps(pencth, pbuf, player_bsinfo->pNalLengthInByte[i]);
                        if (pnal->data_size < 4 || (pnal->data_size > sizeof(popenenc->sps_buf)))
                        {   //ʧ�ܣ�����ԭ����
                            pnal->data_buf = pbuf;
                            pnal->data_size = player_bsinfo->pNalLengthInByte[i];
                        }
                        else
                        {
                            pnal->data_buf = popenenc->sps_buf;
                        }
                    }
                    else
                    {
                        pnal->data_buf = pbuf;
                        pnal->data_size = player_bsinfo->pNalLengthInByte[i];
                    }

                    pnal->frame_end_flg = 1;    //sps pps�����ý�����־

                    pbuf += player_bsinfo->pNalLengthInByte[i];
                }
            }
            ++ ilayer;
        }
    }
    else
    {
        dlog("EncodeParameterSets error: return [%d]!", ienc_frames);
        return -1;
    }
    return 0;
}

static int32_t openh264_force_intra_frame(H264_ENCODER_TH *pencth, int32_t is_idr)
{
    OPENH264_ENOCDER_ST *popenenc = NULL;
    bool  bidr = true;

    if (NULL == pencth)
    {
        elog("openh264_force_intra_frame:input error!");
        return -1;
    }

    popenenc = (OPENH264_ENOCDER_ST *)(pencth->_private_data);

    if (is_idr)
    {
        bidr = true;
    }
    else
    {
        bidr = false;
    }
    return (*(popenenc->pencoder))->ForceIntraFrame(popenenc->pencoder, bidr);
}

static void openh264_reset_frame_rate(H264_ENCODER_TH *pencth, int32_t  frame_rate)
{
    if (NULL == pencth)
    {
        elog("openh264_reset_frame_rate:input error!");
        return ;
    }

    pencth->param.frame_rate = frame_rate;
}

static int32_t openh264_init(H264_ENCODER_TH *pencth, ENCODE_PARAMETER *pparam)
{
    int32_t iret = 0;
    OPENH264_ENOCDER_ST *popenenc = NULL;
    SEncParamExt param_ext;
    int32_t i = 0;
    int32_t i_slice = 0;

    if ((NULL == pencth) || (NULL == pparam))
    {
        elog("openh264_init:input error!");
        return -1;
    }

    memset(&param_ext, 0, sizeof(param_ext));

    popenenc = (OPENH264_ENOCDER_ST *)malloc(sizeof(OPENH264_ENOCDER_ST));
    if (NULL == popenenc)
    {
        elog("openh264_init:alloc memery is fail!");
        return -1;
    }

    memset(popenenc, 0, sizeof(OPENH264_ENOCDER_ST));

    iret = WelsCreateSVCEncoder (&(popenenc->pencoder));
    if (iret)
    {
        elog( "WelsCreateSVCEncoder() failed!!");
        free(popenenc);
        return -1;
    }

    //��ȡĬ�ϲ���
    (*(popenenc->pencoder))->GetDefaultParams(popenenc->pencoder, &param_ext);

    //������������
    param_ext.iUsageType = CAMERA_VIDEO_REAL_TIME;
    param_ext.sSpatialLayers[0].uiProfileIdc  = PRO_BASELINE;
    param_ext.uiIntraPeriod       = 0;            // intra period (multiple of GOP size as desired)
//    param_ext.iNumRefFrame        = 10;    // number of reference frame used  //û��Ӱ�죬��Ĭ��ֵ��

    param_ext.iPicWidth   = pparam->width;    //   actual input picture width
    param_ext.iPicHeight  = pparam->height;    //   actual input picture height

    param_ext.fMaxFrameRate       = pparam->frame_rate;   // maximal frame rate [Hz / fps]

    param_ext.iMultipleThreadIdc      =  pparam->multiple_thread_id;  // 1 # 0: auto(dynamic imp. internal encoder); 1: multiple threads imp. disabled; > 1: count number of threads;

    param_ext.iComplexityMode         =  pparam->complexity_mode; //LOW_COMPLEXITY; //HIGH_COMPLEXITY; //LOW_COMPLEXITY;  //���Ӷ�ģ�ͣ�Խ����Խ��������Խ�ߡ�

    param_ext.iLTRRefNum              = 0;
    param_ext.iLtrMarkPeriod          = 30;   //the min distance of two int32_t references

    param_ext.bEnableSSEI                 = true;
    param_ext.bEnableFrameCroppingFlag    = true; // enable frame cropping flag: true alwayse in application
    // false: Streaming Video Sharing; true: Video Conferencing Meeting;

    /* Deblocking loop filter */
    param_ext.iLoopFilterDisableIdc       = 0;    // 0: on, 1: off, 2: on except for slice boundaries
    param_ext.iLoopFilterAlphaC0Offset    = 0;    // AlphaOffset: valid range [-6, 6], default 0
    param_ext.iLoopFilterBetaOffset       = 0;    // BetaOffset:  valid range [-6, 6], default 0

    /* Rate Control */
    param_ext.iRCMode         = RC_BITRATE_MODE; //RC_BITRATE_MODE; //RC_QUALITY_MODE;  //RC_LOW_BW_MODE������Ϊ��С������ģʽ
    param_ext.iTargetBitrate          = pparam->bitrate;    //��λΪbps������kpbs�� overall target bitrate introduced in RC module
    param_ext.iMaxBitrate             = pparam->max_bitrate;     //û�����á��������Ѿ�����С��ֵ�ˡ������������֡����ή�͡�
    param_ext.iPaddingFlag    = 0;
    param_ext.bEnableFrameSkip        = false;     // frame skipping�Ƿ�����Ϊ�˱���������֡�������Ҫ���á�

    param_ext.bEnableDenoise              = false;    // denoise control
    param_ext.bEnableSceneChangeDetect    = true;     // scene change detection control
    param_ext.bEnableBackgroundDetection  = true;     // background detection control
    param_ext.bEnableAdaptiveQuant        = true;     // adaptive quantization control
    param_ext.bEnableLongTermReference    = false;    // long term reference control
    param_ext.eSpsPpsIdStrategy = INCREASING_ID;     // pSps pPps id addition control
    param_ext.bPrefixNalAddingCtrl        = false;     // prefix NAL adding control�Ƿ����ǰ׺��Ϣ��Ҳ��Ҫ���á�
    param_ext.bSimulcastAVC  = 1;               //���������һ��ʱ���Ƿ�Ը߲�ʹ��SVC�﷨��
    param_ext.iEntropyCodingModeFlag = 0;       //0:CAVLC  1:CABAC.
    param_ext.iSpatialLayerNum        = 1;        //������1������������⡣��Ƶ����number of dependency(Spatial/CGS) layers used to be encoded
    param_ext.iTemporalLayerNum           = 1;        // ������1������������⡣��Ƶ����number of temporal layer specified

    param_ext.iMaxQp = 51;  //pparam->max_qp;
    param_ext.iMinQp = 0; //pparam->min_qp;
    param_ext.iUsageType = CAMERA_VIDEO_REAL_TIME;

    if (pparam->slice_mode == H264_SM_DYN_SLICE)
    {
        param_ext.uiMaxNalSize = pparam->slice_cfg.max_slice_size + 50; //ֻ����SM_DYN_SLICE����������0.��50������nalͷ
    }

    for(i = 0; i < MAX_SPATIAL_LAYER_NUM; i++)
    {
        param_ext.sSpatialLayers[i].iVideoWidth = pparam->width;        // video size in cx specified for a layer
        param_ext.sSpatialLayers[i].iVideoHeight = pparam->height;       // video size in cy specified for a layer
        param_ext.sSpatialLayers[i].uiProfileIdc = PRO_BASELINE;
        //      param.sSpatialLayers[iLayer].uiLevelIdc = LEVEL_1_0;//����û��Ч��
//        param_ext.sSpatialLayers[i].iDLayerQp = 13;//ʹ��Ĭ��ֵ��Ĭ��ֵ��26
        param_ext.sSpatialLayers[i].iSpatialBitrate = pparam->bitrate;   //��Ҫ������������Ǹı�����ʵĵط���
        param_ext.sSpatialLayers[i].iMaxSpatialBitrate = pparam->max_bitrate;
        param_ext.sSpatialLayers[i].fFrameRate = pparam->frame_rate;
        param_ext.sSpatialLayers[i].sSliceCfg.uiSliceMode = pparam->slice_mode;   //�������slice��С��̬����slice������
        param_ext.sSpatialLayers[i].sSliceCfg.sSliceArgument.uiSliceSizeConstraint = pparam->slice_cfg.max_slice_size;  //slice��С
        param_ext.sSpatialLayers[i].sSliceCfg.sSliceArgument.uiSliceNum = pparam->slice_cfg.slice_num;

        for (i_slice = 0; i_slice < MAX_SLICES_NUM_TMP; i_slice++)
        {
            param_ext.sSpatialLayers[i].sSliceCfg.sSliceArgument.uiSliceMbNum[i_slice] = pparam->slice_cfg.slice_mb_num;
        }
    }

    //������־
    CM_RETURN eret = 0;
    WelsTraceCallback callback = openh264_log_callback;
    eret = (*(popenenc->pencoder))->SetOption(popenenc->pencoder, ENCODER_OPTION_TRACE_CALLBACK, &callback);

    eret = (*(popenenc->pencoder))->InitializeExt (popenenc->pencoder, &param_ext);
    if (cmResultSuccess != eret)
    { // SVC encoder initialization
        elog("SVC encoder Initialize failed! Return is :%d\n", eret);
        WelsDestroySVCEncoder(popenenc->pencoder);
        free(popenenc);
        return -1;
    }

    pencth->_private_data = (void *)popenenc;
    memcpy(&(pencth->param), pparam, sizeof(pencth->param));
    pencth->encode_frame = openh264_encode_frame;
    pencth->encode_parameter_sets = openh264_encode_parameter_sets;
    pencth->force_intra_frame = openh264_force_intra_frame;
    pencth->reset_frame_rate = openh264_reset_frame_rate;

    return 0;
}

static void openh264_release(H264_ENCODER_TH *pencth)
{
    OPENH264_ENOCDER_ST *popenenc = NULL;

    if ((NULL == pencth) || (NULL == pencth->_private_data))
    {
        elog("openh264_release:input error!");
        return ;
    }

    popenenc = pencth->_private_data;

    WelsDestroySVCEncoder(popenenc->pencoder);
    free(popenenc);
}

void openh264_encoder_init()
{
    H264_ENCODER_FACTORY *pfact  = NULL;

    pfact = (H264_ENCODER_FACTORY *)malloc(sizeof(H264_ENCODER_FACTORY));
    if (NULL == pfact)
    {
        elog("openh264_encoder_init:malloc memery is fail!");
        return;
    }
    pfact->id = H264_ENCODER_ID_OPENH264;
    pfact->init = openh264_init;
    pfact->release = openh264_release;

    h264_encoder_register(pfact);
}





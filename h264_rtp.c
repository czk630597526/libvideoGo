/*
 * h264_rtp.c
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "h264_rtp.h"
#include "video_rtp.h"
#include "libv_log.h"

static const uint32_t MAX_H264_NAL_LEN  = 1024 * 1024;//初始申请1024k。要保证能够容纳最大的帧，后面不再申请。
static const uint8_t NEW_FRAME_START_SEQUENCE[] = { 0, 0, 0, 1 };
static const uint8_t MIDDLE_FRAME_START_SEQUENCE[] = { 0, 0, 1 };

static uint32_t MAX_ETH_IP_UDP_RTP_HEADER_LEN = 56; //:16 + 20 + 8 + 12
static uint32_t RTP_HEADER_LEN = 12;

#define MAX_STAP_A_NAL_COUNT    32      //STAP_A模式下，一个包最多可以包含包的个数
#define MAX_FU_A_RTP_COUNT    1024      //FU_A模式下，一个包最多可以分为多个包的个数

#define RTP_HEVC_PAYLOAD_HEADER_SIZE       2
#define RTP_HEVC_FU_HEADER_SIZE            1
#define RTP_HEVC_DONL_FIELD_SIZE           2
#define RTP_HEVC_DOND_FIELD_SIZE           1
#define RTP_HEVC_AP_NALU_LENGTH_FIELD_SIZE 2
#define HEVC_SPECIFIED_NAL_UNIT_TYPES      48

//rtp解包结构体
typedef struct __H264_RTP_DEC_ST
{
    VIDEO_CODEC_TYPE codec_type;    //区分是h264或h265
    uint32_t donl_flag;             //donlflag，影响解码。只h265解码时有效
    uint8_t *slice_data;  //[0,0,0,1]Start sequence + [nal header, data]NAL data，可以重复多次。
    uint32_t data_len;      //nal_data中内容的长度，实际使用长度
    uint32_t data_size;     //slice_data内存中的长度。
    uint32_t fua_loss_flg;  //FU_A模式下是否有一个分包丢弃；如果有丢弃，则会丢弃其他所有包
    int32_t pre_seq;       //上一个rtp包序号，用于判断是否丢包
    uint32_t pre_ssrc;       //上一个rtp包ssrc，用于判断是同一个源暂不使用
    uint32_t packet_count;    //packet个数，packets最大值下标。 0: one NAL data is not ready; 1: one NAL data is ready; n:多个包在data中。
    uint32_t pre_mark_flg;    //上一个包的mark标志位
    uint32_t pre_timestamp;    //上一个包时戳
    H264_NAL_DATA  packets[MAX_STAP_A_NAL_COUNT];    //存储解析出来的packet包
}H264_RTP_DEC_ST;

//rtp压包结构体
typedef struct __H264_RTP_ENC_ST
{
    uint8_t *rtp_data;        //rtp包缓存,初始申请长度为：MAX_H264_NAL_LEN
    uint32_t data_len;          //nal_data中内容的长度，实际使用长度
    uint32_t data_size;         //rtp_data内存中的长度。
    uint32_t pload_type;
    uint32_t ssrc;
    uint32_t max_pload_size;       //pload的最大值
    uint16_t cur_seq;
    PACKET_MODE_E packet_mode;      //打包模式

    uint32_t packet_count;    //packet个数，packets最大值下标。
    H264_RTP_PACKET packets[MAX_FU_A_RTP_COUNT];    //存储编码后的RTP包
}H264_RTP_ENC_ST;

H264_RTP_DEC_TH alloc_h264_rtp_dec_th()
{
    H264_RTP_DEC_ST *ph264dec_th = (H264_RTP_DEC_ST *)malloc(sizeof(H264_RTP_DEC_ST));
    if (NULL == ph264dec_th)
    {
        elog("alloc ph264dec_th memery is fail!");
        return NULL;
    }

    memset(ph264dec_th, 0, sizeof(H264_RTP_DEC_ST));

    ph264dec_th->codec_type = VIDEO_CODEC_H264;
    ph264dec_th->packet_count = 0;
    ph264dec_th->data_len = 0;
    ph264dec_th->slice_data = (uint8_t *)malloc(MAX_H264_NAL_LEN);//初始申请。要保证能够容纳最大的帧，后面不够再realloc。
    if (NULL == ph264dec_th->slice_data)
    {
        elog("alloc slice_data memery is fail!");
        free(ph264dec_th);
        return NULL;
    }
    ph264dec_th->data_size = MAX_H264_NAL_LEN;

    ph264dec_th->pre_seq = -1;  //
    ph264dec_th->pre_ssrc = 0;
    ph264dec_th->fua_loss_flg = 0;

    ph264dec_th->pre_mark_flg = 1;  //上一个包的标志应该置为1，这样受到第一个包会认为是新报

    memset(ph264dec_th->packets, 0, sizeof(ph264dec_th->packets));

    return (H264_RTP_DEC_TH )ph264dec_th;
}

void free_h264_rtp_dec_th(H264_RTP_DEC_TH pdec_th)
{
    H264_RTP_DEC_ST *prtpdec_th = (H264_RTP_DEC_ST *)pdec_th;

    if (prtpdec_th)
    {
        if (prtpdec_th->slice_data)
        {
            free(prtpdec_th->slice_data);
        }
        free(prtpdec_th);
    }
}

//情况标志和数据，开始新的rtp解析
static void reset_h264_rtp_dec_th_data(H264_RTP_DEC_ST *pdec_th)
{
    pdec_th->packet_count = 0;
    pdec_th->data_len = 0;
    pdec_th->fua_loss_flg = 0;
}

static int32_t realloc_h264_rtp_dec_nalu_data_buf(H264_RTP_DEC_ST *pdec_th, int32_t data_size)
{
    int32_t new_size  = 0;
    uint8_t *ptmp_buf = NULL;

    new_size = pdec_th->data_size + sizeof(NEW_FRAME_START_SEQUENCE) + ((data_size > (pdec_th->data_size / 2) ? data_size:(pdec_th->data_size / 2)));
    ptmp_buf = (uint8_t *)realloc(pdec_th->slice_data, new_size);
    if (NULL == ptmp_buf)
    {
        elog("Realloc slice_data is fail:new size[%d].", new_size);
        return -1;
    }
    pdec_th->slice_data = ptmp_buf;
    pdec_th->data_size = new_size;

    return 0;
}

//单包模式下添加一个nalu，包含起始码。一个nalu就是一帧
static int32_t h264_rtp_dec_add_one_nalu_one_frame(H264_RTP_DEC_ST *pdec_th, uint8_t nal_type, uint32_t timestamp,
                                                    int32_t data_size, const uint8_t *data_buf)
{

    if (pdec_th->packet_count >= MAX_STAP_A_NAL_COUNT)
    {
        elog("Funtion reset_h264_rtp_dec_add_packet:the packet_count[%d] is larger than MAX_STAP_A_NAL_COUNT[%d]",
                pdec_th->packet_count, MAX_STAP_A_NAL_COUNT);
        return -1;
    }

    if ((pdec_th->data_len + sizeof(NEW_FRAME_START_SEQUENCE) + data_size) >= pdec_th->data_size)
    {
        if (0 != realloc_h264_rtp_dec_nalu_data_buf(pdec_th, data_size))
        {
            elog("Realloc slice_data is fail:new size[%d].", pdec_th->data_size);
            return -1;
        }
    }

    H264_NAL_DATA *pnal_pack =  (H264_NAL_DATA *)&(pdec_th->packets[pdec_th->packet_count]);
    pnal_pack->used_flag = 1;
    pnal_pack->nal_type = nal_type;
    pnal_pack->timestamp = timestamp;
    pnal_pack->data_size = data_size + sizeof(NEW_FRAME_START_SEQUENCE);
    pnal_pack->data_buf = pdec_th->slice_data + pdec_th->data_len;
    pnal_pack->frame_end_flg = 1;

    memcpy(pdec_th->slice_data + pdec_th->data_len, NEW_FRAME_START_SEQUENCE, sizeof(NEW_FRAME_START_SEQUENCE));
    pdec_th->data_len += sizeof(NEW_FRAME_START_SEQUENCE);
    memcpy(pdec_th->slice_data + pdec_th->data_len, data_buf, data_size);
    pdec_th->data_len += data_size;

    pdec_th->packet_count += 1;

    return 0;

}

//添加一个nalu起始码，如果是新的一帧开始（is_new_frame == 1），则填00 00 00 01，否则填00 00 01
static int32_t h264_rtp_dec_add_nalu_start_sequence(H264_RTP_DEC_ST *pdec_th, int32_t is_new_frame,
                                            uint8_t nal_type, uint32_t timestamp)
{
    if (NULL == pdec_th)
    {
        elog("h264_rtp_dec_add_nalu_start_sequence:input is error!");
        return -1;
    }

    if (pdec_th->packet_count >= MAX_STAP_A_NAL_COUNT)
    {
        elog("Funtion h264_rtp_dec_add_nalu_start_sequence:the packet_count[%d] is larger than MAX_STAP_A_NAL_COUNT[%d]",
                pdec_th->packet_count, MAX_STAP_A_NAL_COUNT);
        return -1;
    }

    if ((pdec_th->data_len + sizeof(NEW_FRAME_START_SEQUENCE)) >= pdec_th->data_size)
    {
        if (0 != realloc_h264_rtp_dec_nalu_data_buf(pdec_th, sizeof(NEW_FRAME_START_SEQUENCE)))
        {
            elog("Realloc slice_data is fail:new size[%d].", pdec_th->data_size);
            return -1;
        }
    }

//    if (pdec_th->packet_count > 0)
//    {
//        pdec_th->packets[pdec_th->packet_count - 1].frame_end_flg = 1;//上一个结束标志置1
//    }

    H264_NAL_DATA *pnal_pack =  (H264_NAL_DATA *)&(pdec_th->packets[pdec_th->packet_count]);
    if (is_new_frame)
    {
        pnal_pack->used_flag = 1;
        pnal_pack->nal_type = nal_type;
        pnal_pack->timestamp = timestamp;
        pnal_pack->data_size = sizeof(NEW_FRAME_START_SEQUENCE);
        pnal_pack->data_buf = pdec_th->slice_data + pdec_th->data_len;
        pnal_pack->frame_end_flg = 0;

        memcpy(pdec_th->slice_data + pdec_th->data_len, NEW_FRAME_START_SEQUENCE, sizeof(NEW_FRAME_START_SEQUENCE));
        pdec_th->data_len += sizeof(NEW_FRAME_START_SEQUENCE);
    }
    else
    {
        pnal_pack->used_flag = 1;
        pnal_pack->nal_type = nal_type;
        pnal_pack->timestamp = timestamp;
        pnal_pack->data_size += sizeof(MIDDLE_FRAME_START_SEQUENCE);
        pnal_pack->frame_end_flg = 0;

        memcpy(pdec_th->slice_data + pdec_th->data_len, MIDDLE_FRAME_START_SEQUENCE, sizeof(MIDDLE_FRAME_START_SEQUENCE));
        pdec_th->data_len += sizeof(MIDDLE_FRAME_START_SEQUENCE);
    }
    return 0;
}

//添加当前帧的结束标志
static int32_t h264_rtp_dec_add_frame_end_flag(H264_RTP_DEC_ST *pdec_th)
{
    pdec_th->fua_loss_flg = 0;

    if (pdec_th->packet_count >= MAX_STAP_A_NAL_COUNT)
    {
        elog("Funtion h264_rtp_dec_add_nalu_start_sequence:the packet_count[%d] is larger than MAX_STAP_A_NAL_COUNT[%d]",
                pdec_th->packet_count, MAX_STAP_A_NAL_COUNT);
        return -1;
    }
    H264_NAL_DATA *pnal_pack =  (H264_NAL_DATA *)&(pdec_th->packets[pdec_th->packet_count]);
    if (pnal_pack->used_flag)
    {
        pnal_pack->frame_end_flg = 1;
        pdec_th->packet_count += 1;
    }
    return 0;
}

//添加一帧的部分数据
static int32_t h264_rtp_dec_add_part_data_of_frame(H264_RTP_DEC_ST *pdec_th, int32_t data_size, const uint8_t *data_buf)
{
    if (pdec_th->packet_count >= MAX_STAP_A_NAL_COUNT)
    {
        elog("Funtion h264_rtp_dec_add_nalu_start_sequence:the packet_count[%d] is larger than MAX_STAP_A_NAL_COUNT[%d]",
                pdec_th->packet_count, MAX_STAP_A_NAL_COUNT);
        return -1;
    }

    if ((pdec_th->data_len + data_size) >= pdec_th->data_size)
    {
        if (0 != realloc_h264_rtp_dec_nalu_data_buf(pdec_th, data_size))
        {
            elog("Realloc slice_data is fail:new size[%d].", pdec_th->data_size);
            return -1;
        }
    }

    H264_NAL_DATA *pnal_pack =  (H264_NAL_DATA *)&(pdec_th->packets[pdec_th->packet_count]);
    pnal_pack->data_size += data_size;
    memcpy(pdec_th->slice_data + pdec_th->data_len, data_buf, data_size);
    pdec_th->data_len += data_size;

    return 0;
}

//rtp_buf:包含rtp包头.返回值：获得包的个数.-1表示解析失败
int32_t h264_dec_rtp(H264_RTP_DEC_TH pdec_th, const uint8_t *rtp_buf, int32_t buf_len)
{
    const uint8_t * pnalbuf = NULL;
    uint8_t nal = 0;
    uint8_t type = 0;
    uint8_t nal_type = 0;
    uint8_t cur_mark_flg = 0;   //当前包mark标志
    uint8_t pre_mark_flg = 0;   //上一个包的mark标志
    int32_t nal_len = 0;
    H264_RTP_DEC_ST *prtp_dec_th = (H264_RTP_DEC_ST *)pdec_th;
    uint16_t  seq = 0;
    uint32_t  pre_timestamp = 0;
    uint32_t  timestamp = 0;
    uint32_t  is_loss_flg = 0;  //是否发生过丢包

    if ((NULL == pdec_th) || (NULL == rtp_buf))
    {
        return -1;
    }
    if (buf_len < RTP_HEADER_LEN + 1)
    {
        elog("h264 rtp dec: buf len[%d] is less than 13", buf_len);
        return -1;
    }

    //获取序号和时戳
    seq = ntohs(*(uint16_t *)(&rtp_buf[2]));
    timestamp = ntohl(*(uint32_t *)(&rtp_buf[4]));
    if ((prtp_dec_th->pre_seq + 1 != seq) && (prtp_dec_th->pre_seq != -1))   //有跳跃且非初始值；
    {
        if ((prtp_dec_th->pre_seq == 65535) && (seq == 0))  //临界值，则不算丢包
        {
        }
        else
        {
            wlog("There is lose packet or sequence is error.pre[%d], cur[%d]", prtp_dec_th->pre_seq, seq);
            is_loss_flg = 1;
        }
    }
    prtp_dec_th->pre_seq = seq; //记录当前序号
    pre_timestamp = prtp_dec_th->pre_timestamp; //记录上一个时戳
    prtp_dec_th->pre_timestamp = timestamp;

    pnalbuf = rtp_buf + RTP_HEADER_LEN;//首先跳过12个字节的rtp包头。
    nal_len = buf_len - RTP_HEADER_LEN;

    nal  = pnalbuf[0];
    type = nal & 0x1f;//取后五位，表示是哪种类型的打包方式
    nal_type = type;

    cur_mark_flg = rtp_buf[1] & 0x80;    //取第一位的值，为mark标志位
    pre_mark_flg = prtp_dec_th->pre_mark_flg;
    prtp_dec_th->pre_mark_flg = cur_mark_flg;   //更改对象记录

    /*
    Type   Packet      Type name
    ---------------------------------------------------------
    0      undefined                                    -
    1-23   NAL unit    Single NAL unit packet per H.264
    24     STAP-A     Single-time aggregation packet
    25     STAP-B     Single-time aggregation packet
    26     MTAP16    Multi-time aggregation packet
    27     MTAP24    Multi-time aggregation packet
    28     FU-A      Fragmentation unit
    29     FU-B      Fragmentation unit
    30-31  undefined                                    -
    */

    /* Simplify the case (these are all the nal types used internally by
     * the h264 codec). */
    if (type >= 1 && type <= 23)
    {
        type = 1;
    }

    switch (type)
    {
        case 0:                    // undefined, but pass them through
        case 1:

            //单包模式：
            //只有在1，或5，视频帧的时候，把mark标志作为判断是否结束分包
            if ((nal_type == 1 ) || (nal_type == 5))
            {
                if (pre_mark_flg && cur_mark_flg)  //上一个标志和当前标志都为1，说明是单个包
                {
                    reset_h264_rtp_dec_th_data(prtp_dec_th);
                    h264_rtp_dec_add_one_nalu_one_frame(prtp_dec_th, nal & 0x1f, timestamp, nal_len, pnalbuf);
                }
                else if (pre_mark_flg && !cur_mark_flg)  //上一个包标志为1，当前为0，说明是新的帧开始
                {
                    reset_h264_rtp_dec_th_data(prtp_dec_th);
                    h264_rtp_dec_add_nalu_start_sequence(prtp_dec_th, 1, nal & 0x1f, timestamp);
                    h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, nal_len, pnalbuf);
                }
                else if (!pre_mark_flg && !cur_mark_flg) //上一个标志和当前标志都为0，说明是一帧的中间包
                {
                    //这里要判断时戳。两个时戳不同，则是新的帧，发生了丢包可能
                    if (pre_timestamp == timestamp) //时戳相同，则是同一帧的中间包。
                    {
                        //添加nal 头：00 00 01
                        h264_rtp_dec_add_nalu_start_sequence(prtp_dec_th, 0, nal & 0x1f, timestamp);
                        h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, nal_len, pnalbuf);
                    }
                    else //可能发生了丢包，最后一个带有mark的包没有了，开始新的包
                    {
                        //首先将上一个包结束
                        h264_rtp_dec_add_frame_end_flag(prtp_dec_th);

                        //开始一个新的包
                        h264_rtp_dec_add_nalu_start_sequence(prtp_dec_th, 1, nal & 0x1f, timestamp);
                        h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, nal_len, pnalbuf);
                    }
                }
                else if (!pre_mark_flg && cur_mark_flg) //上一个标志0， 当前标志为1，说明是一帧结束包
                {
                    //这里要判断时戳。两个时戳不同，怎么是新的帧，发生了丢包可能
                    if (pre_timestamp == timestamp) //时戳相同，则是同一帧的结束包。
                    {
                        //添加nal 头：00 00 01
                        h264_rtp_dec_add_nalu_start_sequence(prtp_dec_th, 0, nal & 0x1f, timestamp);
                        h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, nal_len, pnalbuf);
                        h264_rtp_dec_add_frame_end_flag(prtp_dec_th);
                    }
                    else //可能发生了丢包，最后一个带有mark的包没有了，开始新的包
                    {
                        //首先将上一个包结束
                        h264_rtp_dec_add_frame_end_flag(prtp_dec_th);

                        //开始一个新的包
                        h264_rtp_dec_add_one_nalu_one_frame(prtp_dec_th, nal & 0x1f, timestamp, nal_len, pnalbuf);
                    }
                }
            }
            else
            {
                //其他类型的包，不管类型，都认为是新的包。并把上一个包mark标志置为1,表示下一个包是新的开始
                //首先将上一个包结束
                h264_rtp_dec_add_frame_end_flag(prtp_dec_th);

                h264_rtp_dec_add_one_nalu_one_frame(prtp_dec_th, nal & 0x1f, timestamp, nal_len, pnalbuf);
                prtp_dec_th->pre_mark_flg = 1;
            }

            return prtp_dec_th->packet_count;
            break;

        case 24:                   // STAP-A (one packet, multiple nals)
            pnalbuf++;
            nal_len--;              // skip the fu_header

            reset_h264_rtp_dec_th_data(prtp_dec_th);

            while (nal_len > 2)
            {
                uint16_t nal_size = pnalbuf[0] * 256 + pnalbuf[1];
                // sikp the size :two byte
                pnalbuf     += 2;
                nal_len -= 2;

                if (nal_size <= nal_len)
                {
                    h264_rtp_dec_add_one_nalu_one_frame(prtp_dec_th, pnalbuf[0] & 0x1f, timestamp, nal_size, pnalbuf);   //这里的type获取要看每个nal 的

                    pnalbuf += nal_size;
                    nal_len -= nal_size;
                }
                else
                {
                    elog("nal size exceeds length: %d %d.", nal_size, nal_len);
                    break;  //break while
                }
            }
            return prtp_dec_th->packet_count;
            break;

        case 25:                   // STAP-B
        case 26:                   // MTAP-16
        case 27:                   // MTAP-24
        case 29:                   // FU-B
            //这里暂不支持。
            elog("H264 pack no suport STAP-B MTAP-16 MTAP-24 FU-B.");
            return -1;
            break;

        case 28:                   // FU-A (fragmented nal)
            pnalbuf++;
            nal_len--;              // skip the fu_indicator
            //要考虑的一种特殊情况是，一帧中有多个slice都是用fu_a编码的。最后要输出为一个nalu，而不是多个
            if (nal_len > 1)
            {
                uint8_t fu_indicator      = nal;
                uint8_t fu_header         = pnalbuf[0];
                uint8_t start_bit         = fu_header >> 7;
                uint8_t end_bit = (fu_header & 0x40) >> 6;
                uint8_t nal_unit_header = (fu_indicator & 0xe0) | (fu_header & 0x1f);//fu_indicator的前三位加上fu_header的后五位

                pnalbuf++;  //跳过头
                nal_len--;

                if (start_bit)//起始包
                {
                    if (pre_mark_flg)   //上一帧为结束帧，这一帧为新帧
                    {
                        reset_h264_rtp_dec_th_data(prtp_dec_th);
                        h264_rtp_dec_add_nalu_start_sequence(pdec_th, 1, nal_unit_header & 0x1f, timestamp);
                    }
                    else if (pre_timestamp != timestamp)//上一阵mark为0，但是这一帧时戳不同了，要停止上一帧，开始新帧
                    {
                        h264_rtp_dec_add_frame_end_flag(pdec_th);
                        h264_rtp_dec_add_nalu_start_sequence(pdec_th, 1, nal_unit_header & 0x1f, timestamp);
                    }
                    else    //否则，就是中间的帧了
                    {
                        h264_rtp_dec_add_nalu_start_sequence(pdec_th, 0, nal_unit_header & 0x1f, timestamp);
                    }
                    //首先添加nal头，这要单独添加，因为不可以更改原有的数据。
                    h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, sizeof(nal_unit_header), &nal_unit_header);

                    h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, nal_len, pnalbuf);
                }
                else if (!end_bit) //中间包
                {
                    if (pre_timestamp != timestamp)//上一阵mark为0，但是这一帧时戳不同了，要停止上一帧，开始新帧
                    {
                        h264_rtp_dec_add_frame_end_flag(pdec_th);
                        h264_rtp_dec_add_nalu_start_sequence(pdec_th, 1, nal_unit_header & 0x1f, timestamp);

                        //首先添加nal头，这要单独添加，因为不可以更改原有的数据。
                        h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, sizeof(nal_unit_header), &nal_unit_header);
                    }

                    if (is_loss_flg)
                    {
                        prtp_dec_th->fua_loss_flg = 1;
                    }

                    if (prtp_dec_th->fua_loss_flg)
                    {
//                        dlog("The Fua loss flg is True, so lose this packet!");
//                        return prtp_dec_th->packet_count;
                    }
                    h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, nal_len, pnalbuf);
                }
                else if (end_bit)//结束包
                {
                    if (pre_timestamp != timestamp)//上一阵mark为0，但是这一帧时戳不同了，要停止上一帧，开始新帧
                    {
                        h264_rtp_dec_add_frame_end_flag(pdec_th);
                        h264_rtp_dec_add_nalu_start_sequence(pdec_th, 1, nal_unit_header & 0x1f, timestamp);

                        //首先添加nal头，这要单独添加，因为不可以更改原有的数据。
                        h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, sizeof(nal_unit_header), &nal_unit_header);
                    }
                    if (is_loss_flg)
                    {
                        prtp_dec_th->fua_loss_flg = 1;
                    }
                    if (prtp_dec_th->fua_loss_flg)
                    {
//                        dlog("The Fua loss flg is True, so lose this packet!");
//                        return prtp_dec_th->packet_count;
                    }
                    h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, nal_len, pnalbuf);
                    if (cur_mark_flg)
                    {
                        h264_rtp_dec_add_frame_end_flag(pdec_th);
                    }
                }
            }
            else
            {
                elog("Too short data for FU-A H264 RTP packet");
                return -1;
            }
            return prtp_dec_th->packet_count;
            break;

        case 30:                   // undefined
        case 31:                   // undefined
        default:
            break;
    }
    return -1;
}


//函数返回数据的指针，没有数据则返回空。这个函数要一直调用，直到返回null为止，否则数据会被覆盖
//返回的指针或指针中的缓冲区不可以作为上层的存储缓冲区，如果需要保存一段时间，则需要重新申请新的缓冲区并拷贝。
//used_flag字段返回前会被置为0.
H264_NAL_DATA * h264_dec_rtp_get_nal_data(H264_RTP_DEC_TH pdec_th)
{
    H264_RTP_DEC_ST *prtp_dec_th = (H264_RTP_DEC_ST *)pdec_th;
    int32_t i = 0;

    if (NULL == pdec_th)
    {
        elog("get_h264_slice_data args is error:[%p]", pdec_th);
        return NULL;
    }

    if (prtp_dec_th->packet_count < 0)
    {
        return NULL;
    }

    if (prtp_dec_th->packet_count > MAX_STAP_A_NAL_COUNT)
    {
        elog("ph264th->data_count is error:[%d]", prtp_dec_th->packet_count);
        return NULL;
    }

    for (i = 0; i < MAX_STAP_A_NAL_COUNT; i++)
    {
        if (prtp_dec_th->packets[i].used_flag && (prtp_dec_th->packets[i].frame_end_flg))
        {
            prtp_dec_th->packets[i].used_flag = 0;
            if (prtp_dec_th->packet_count != 0)
            {
                prtp_dec_th->packet_count -= 1;
            }
            return &(prtp_dec_th->packets[i]);
        }
        else if ((i != 0) && prtp_dec_th->packets[i].used_flag && !(prtp_dec_th->packets[i].frame_end_flg))//在使用，没有结束
        {
            if (prtp_dec_th->packets[i].data_size <= prtp_dec_th->data_size)
            {
                //移到第一个位置上
                memcpy(&(prtp_dec_th->packets[0]), &(prtp_dec_th->packets[i]), sizeof(prtp_dec_th->packets[0]));

                //移动内存：
                memcpy(prtp_dec_th->slice_data, prtp_dec_th->packets[i].data_buf, prtp_dec_th->packets[i].data_size);

                prtp_dec_th->packets[0].data_buf = prtp_dec_th->slice_data;
                prtp_dec_th->packets[0].data_size = prtp_dec_th->packets[i].data_size;

                prtp_dec_th->packet_count = 0;
                prtp_dec_th->data_len = prtp_dec_th->packets[0].data_size;

            }

            memset(&(prtp_dec_th->packets[i]), 0, sizeof((prtp_dec_th->packets[i])));
            return NULL;
        }
    }

    return NULL;
}

/*
 *pload_type:负载类型；s_seq：起始序列号,使用随机数作为开始；
 *ssrc：同步源id;frame_rate：帧率，目前仅支持整数，后面可能会支持浮点数
 *mtu:单个ip包的最大限制，建议1498，此值包括ip包头，udp包头，rtp头的长度。
 * */
H264_RTP_ENC_TH * alloc_h264_rtp_enc_th(uint8_t pload_type, uint16_t s_seq, uint32_t ssrc, uint32_t mtu, PACKET_MODE_E pm)
{
    if (mtu <= MAX_ETH_IP_UDP_RTP_HEADER_LEN)
    {
        elog("mtu len is error[%d]! ", mtu);
        return NULL;
    }

    H264_RTP_ENC_ST *penc_th = (H264_RTP_ENC_ST *)malloc(sizeof(H264_RTP_ENC_ST));
    if (NULL == penc_th)
    {
        elog("alloc penc_th memery is fail! ");
        return NULL;
    }

    penc_th->data_len = 0;
    penc_th->rtp_data = (uint8_t *)malloc(MAX_H264_NAL_LEN);//初始申请。要保证能够容纳最大的帧，后面不再申请。
    if (NULL == penc_th->rtp_data)
    {
        elog("alloc rtp_data memery is fail! ");
        free(penc_th);
        return NULL;
    }
    penc_th->data_size = MAX_H264_NAL_LEN;

    penc_th->pload_type = pload_type;
    penc_th->cur_seq = s_seq;
    penc_th->ssrc = ssrc;
    penc_th->max_pload_size = mtu - MAX_ETH_IP_UDP_RTP_HEADER_LEN;
    penc_th->packet_mode = pm;

    penc_th->packet_count = 0;
    memset(penc_th->packets, 0, sizeof(penc_th->packets));

    return (H264_RTP_DEC_TH *)penc_th;
}
void free_h264_rtp_enc_th(H264_RTP_ENC_TH ph264dec_th)
{
    H264_RTP_ENC_ST *penc_th = (H264_RTP_ENC_ST *)ph264dec_th;

    if (NULL == penc_th)
    {
        elog("Funtion free_h264_rtp_enc_th input is error! ");
        return ;
    }

    if (NULL != penc_th->rtp_data)
    {
        free(penc_th->rtp_data);
    }
    free(penc_th);

}

//last:是否是一个包的结束，0或1，需要设置rtp的mark标志,pl_buf:payload
//pre_buf:在payload_buf前先拷贝这一块buf。可以为空,此时pre_buf_len要为0。
static int32_t h264_enc_rtp_add_pack(H264_RTP_ENC_ST *penc_th, const uint8_t *pre_buf, int32_t pre_buf_len,
                                    const uint8_t *payload_buf, int32_t buf_len, uint32_t timestamp, int32_t last)
{
    uint8_t *pbuf = NULL;
    uint16_t temp_seq = 0;
    uint8_t *ptemp = NULL;
    uint32_t itemp = 0;

    uint32_t  new_size = 0;
    uint8_t * ptmp_buf = NULL;
    H264_RTP_PACKET *ppack = NULL;

    if ((NULL == penc_th) || (NULL == payload_buf))
    {
        elog("The input is error:h264_enc_rtp_add_rtp. ");
        return -1;
    }

    if (penc_th->packet_count >= MAX_FU_A_RTP_COUNT)
    {
        elog("The penc_th->packet_count[%d] is larger than MAX_FU_A_RTP_COUNT[%d]. ", penc_th->packet_count , MAX_FU_A_RTP_COUNT);
        return -1;
    }

    if ((penc_th->data_len + RTP_HEADER_LEN + buf_len + pre_buf_len)  >= penc_th->data_size)
    {
        new_size = penc_th->data_size + penc_th->data_size / 2;
        ptmp_buf = (uint8_t *)realloc(penc_th->rtp_data, new_size); //这里一定不能把realloc的返回值直接赋给penc_th->rtp_data。
        if (NULL == ptmp_buf)
        {
            elog("Realloc rtp_data is fail:new size[%d]. ", new_size);
            return -1;
        }
        penc_th->rtp_data = ptmp_buf;
        penc_th->data_size = new_size;
    }

    pbuf = penc_th->rtp_data + penc_th->data_len;

    pbuf[0] = 0x80;
    pbuf[1] = (penc_th->pload_type & 0x7f) | ((last & 0x01) << 7);//第7位是makr标志位。

    temp_seq = htons(penc_th->cur_seq);
    ptemp = (uint8_t *)&temp_seq;
    pbuf[2] = ptemp[0];
    pbuf[3] = ptemp[1];

    penc_th->cur_seq += 1;  //序号 + 1

    itemp = htonl(timestamp);
    ptemp = (uint8_t *)&itemp;
    pbuf[4] = ptemp[0];
    pbuf[5] = ptemp[1];
    pbuf[6] = ptemp[2];
    pbuf[7] = ptemp[3];

    itemp = htonl(penc_th->ssrc);
    ptemp = (uint8_t *)&itemp;
    pbuf[8] = ptemp[0];
    pbuf[9] = ptemp[1];
    pbuf[10] = ptemp[2];
    pbuf[11] = ptemp[3];

    if (pre_buf != NULL)
    {
        memcpy(&pbuf[RTP_HEADER_LEN], pre_buf, pre_buf_len);
        penc_th->data_len  += RTP_HEADER_LEN + pre_buf_len;

        memcpy(&pbuf[RTP_HEADER_LEN + pre_buf_len], payload_buf, buf_len);
        penc_th->data_len  += buf_len;
    }
    else
    {
        memcpy(&pbuf[RTP_HEADER_LEN], payload_buf, buf_len);
        penc_th->data_len  += RTP_HEADER_LEN + buf_len;
    }

    ppack = &(penc_th->packets[penc_th->packet_count]);
    ppack->used_flag = 1;
    ppack->seq = penc_th->cur_seq - 1;  //前面已经加过1了，所以这里要减一
    ppack->data_buf = pbuf;
    if (NULL != pre_buf)
    {
        ppack->data_size = RTP_HEADER_LEN + buf_len + pre_buf_len;
    }
    else
    {
        ppack->data_size = RTP_HEADER_LEN + buf_len;
    }

    penc_th->packet_count++;

    return 0;

}

static int32_t h264_enc_rtp_one_nalu_is_clean(H264_RTP_ENC_TH ph264enc_th, const uint8_t *nal_buf, int32_t buf_len, uint32_t timestamp,
                                              uint32_t mark_flg, uint32_t clean_flg)
{
    H264_RTP_ENC_ST *penc_th = (H264_RTP_ENC_ST *)ph264enc_th;
    const uint8_t *pbuf = nal_buf;
    uint8_t type = 0;
    uint8_t nri = 0;
    uint8_t fu_head[2] = "";
    int32_t tmp_buf_len = buf_len;

    if ((NULL == ph264enc_th) || (NULL == nal_buf) || (tmp_buf_len < 6))
    {
        elog("The input is error:h264_enc_rtp_send. buf_len[%d]", tmp_buf_len);
        return -1;
    }

    if (nal_buf[2] == 0)    //开始码为：0，0，0，1
    {
        pbuf = nal_buf + 4;
        tmp_buf_len = tmp_buf_len - 4;
    }
    else    //开始码为0，0，1
    {
        pbuf = nal_buf + 3;
        tmp_buf_len = tmp_buf_len - 3;
    }

    /*
      +---------------+
      |0|1|2|3|4|5|6|7|
      +-+-+-+-+-+-+-+-+
      |F|NRI| Type    |
      +---------------+
    */
    type = pbuf[0] & 0x1F;     //首字节后五位:目前，支持type是1或者5的时候才增加时戳
    nri = pbuf[0] & 0x60;      //首字节第二位和第三位

    //收到一个包后，对编码结构体进行初始化，防止有残留数据
    //memset(penc_th->rtp_data, 0, penc_th->data_size);   //初始化缓存不进行，因为太大，会影响性能。依靠长度来保证不会拷贝错误。
    //memset(penc_th->packets, 0, sizeof(penc_th->packets));    //初始化缓存不进行，因为太大，会影响性能。依靠长度来保证不会拷贝错误。
    if (clean_flg)
    {
        penc_th->data_len = 0;
        penc_th->packet_count = 0;
    }

    if (penc_th->packet_mode == PACKET_MODE_SINGLE)     //如果上层指定，则强制用单包模式
    {
//        printf("buf len [%d]\n", buf_len);
        h264_enc_rtp_add_pack(penc_th, NULL, 0, pbuf, tmp_buf_len, timestamp, mark_flg);
    }
    else if (((uint32_t)tmp_buf_len) <= penc_th->max_pload_size)     //非交错模式，buf长度小于mtu，采用单个包模式
    {
        h264_enc_rtp_add_pack(penc_th, NULL, 0, pbuf, tmp_buf_len, timestamp, mark_flg);
    }
    else    //非交错模式，buf长度大于mtu，采用FU-A模式
    {
      /*
      +---------------+
      |0|1|2|3|4|5|6|7|
      +-+-+-+-+-+-+-+-+
      |F|NRI|  Type   |
      +---------------+
      FU指示字节的类型域 Type=28表示FU-A。。NRI域的值必须根据分片NAL单元的NRI域的值设置。
      FU header的格式如下：
      +---------------+
      |0|1|2|3|4|5|6|7|
      +-+-+-+-+-+-+-+-+
      |S|E|R|  Type   |
      +---------------+
      S: 1 bit
               当设置成1,开始位指示分片NAL单元的开始。当跟随的FU荷载不是分片NAL单元荷载的开始，开始位设为0。
      E: 1 bit
                当设置成1, 结束位指示分片NAL单元的结束，即, 荷载的最后字节也是分片NAL单元的最后一个字节。当跟随的FU荷载不是分片NAL单元的最后分片,结束位设置为0。
      R: 1 bit
                保留位必须设置为0，接收者必须忽略该位。
       Type: 5 bits
       */
        pbuf = pbuf + 1;    //跳过头，因为要重新组头。
        tmp_buf_len--;          //不包括2字节的包头

        fu_head[0] = 28;       //组fu_a的包头
        fu_head[0] |= nri;     //级别
        fu_head[1] = type;
        fu_head[1] |= 1 << 7;  //设置S标志为1

        while (((uint32_t)tmp_buf_len + sizeof(fu_head)) > penc_th->max_pload_size)
        {
            h264_enc_rtp_add_pack(penc_th, fu_head, sizeof(fu_head), pbuf,
                                  penc_th->max_pload_size - sizeof(fu_head), timestamp, 0);

            pbuf += penc_th->max_pload_size - sizeof(fu_head);
            tmp_buf_len -= penc_th->max_pload_size - sizeof(fu_head);

            fu_head[0] = 28;       //把一个包头分成两个包头
            fu_head[0] |= nri;     //级别
            fu_head[1] = type;
            fu_head[1] &= ~(1 << 7);   //设置S标志为0
        }
        fu_head[1] |= 1 << 6;    //设置E标志为1
        h264_enc_rtp_add_pack(penc_th, fu_head, sizeof(fu_head), pbuf, tmp_buf_len, timestamp, mark_flg);
    }
    return penc_th->packet_count;
}

int32_t h264_enc_rtp_one_nalu(H264_RTP_ENC_TH ph264enc_th, const uint8_t *nal_buf, int32_t buf_len, uint32_t timestamp, uint32_t mark_flg)
{
    return h264_enc_rtp_one_nalu_is_clean(ph264enc_th, nal_buf, buf_len, timestamp, mark_flg, 1);   //必须清空
}

static const uint8_t *avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
    const uint8_t *a = p + 4 - ((intptr_t)p & 3);

    for (end -= 3; p < a && p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    for (end -= 3; p < end; p += 4) {
        uint32_t x = *(uint32_t*)p;
//      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
//      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
        if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
            if (p[1] == 0) {
                if (p[0] == 0 && p[2] == 1)
                    return p;
                if (p[2] == 0 && p[3] == 1)
                    return p+1;
            }
            if (p[3] == 0) {
                if (p[2] == 0 && p[4] == 1)
                    return p+2;
                if (p[4] == 0 && p[5] == 1)
                    return p+3;
            }
        }
    }

    for (end += 3; p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    return end + 3;
}

static const uint8_t *avc_find_startcode(const uint8_t *p, const uint8_t *end){
    const uint8_t *out= avc_find_startcode_internal(p, end);
    if(p<out && out<end && !out[-1]) out--;
    return out;
}

/*
int32_t h264_enc_rtp_access_unit(H264_RTP_ENC_TH ph264enc_th, uint8_t *an_buf, int32_t buf_len, uint32_t timestamp)
{
    uint32_t count = 0;
    int32_t ibuf_pos = 0;
    int32_t mark_flag = 0;
    int32_t i = 0;
    int32_t islice_size = 0;

    if ((NULL == an_buf) || (buf_len <= 0))
    {
        wlog("h264_enc_rtp_access_unit input is error! [%p] [%d]", an_buf, buf_len);
        return -1;
    }

    while (ibuf_pos < buf_len)
    {
        for (i = 0; i + ibuf_pos + 4 < buf_len; i++)
        {
            if ((an_buf[ibuf_pos + i] == 0 && an_buf[ibuf_pos + i + 1] == 0 && an_buf[ibuf_pos + i + 2] == 0 && an_buf[ibuf_pos + i + 3] == 1 )
               || (an_buf[ibuf_pos + i] == 0 && an_buf[ibuf_pos + i + 1] == 0 && an_buf[ibuf_pos + i + 2] == 1 ))
            {
                if (i < 4)
                {
                    continue;
                }
                printf("-------------------------   %d  %d\n", ibuf_pos + i , an_buf[ibuf_pos + i + 2]);
                if (an_buf[ibuf_pos + i + 2] == 0)
                {
                    mark_flag = 1;
                }
                else
                {
                    mark_flag = 0;
                }
                break;
            }
        }

        if (i + ibuf_pos + 4 >= buf_len)    //说明已经超过结尾
        {
            islice_size = buf_len - ibuf_pos;
            mark_flag = 1;
        }
        else
        {
            islice_size = i;
        }
        printf("111111111111111         %d     mark_flag  %d\n", islice_size, mark_flag);
        count += h264_enc_rtp_one_nalu(ph264enc_th, an_buf + ibuf_pos, islice_size, timestamp, mark_flag);

        ibuf_pos += islice_size;
    }
    return count;
}
 *
 */

int32_t h264_enc_rtp_access_unit(H264_RTP_ENC_TH ph264enc_th, const uint8_t *an_buf, int32_t buf_len, uint32_t timestamp)
{
    const uint8_t *r = NULL;
    const uint8_t *end = an_buf + buf_len;
    const uint8_t *r1 = NULL;
    const uint8_t *start = NULL;
    uint32_t count = 0;
    uint32_t is_first = 1;

    r = avc_find_startcode(an_buf, end);

    while (r < end)
    {
        start = r;

        while (!*(r++));    // 跳过nalu前缀， 找到nalu第一个字节
        r1 = avc_find_startcode(r, end);    //找到下一个

        if (is_first)
        {
            count = h264_enc_rtp_one_nalu_is_clean(ph264enc_th, start, r1 - start, timestamp, r1 == end, 1);
            is_first = 0;
        }
        else
        {
            count = h264_enc_rtp_one_nalu_is_clean(ph264enc_th, start, r1 - start, timestamp, r1 == end, 0);
        }

        r = r1;
    }
    return count;
}


H264_RTP_PACKET * h264_enc_rtp_get_rtp_data(H264_RTP_ENC_TH  penc_th)
{
    H264_RTP_ENC_ST *prtp_enc_th = (H264_RTP_ENC_ST *)penc_th;
    int32_t i = 0;

    if (NULL == penc_th)
    {
        elog("h264_enc_rtp_get_rtp_data args is error:[%p] ", penc_th);
        return NULL;
    }

    if (prtp_enc_th->packet_count <= 0)
    {
        return NULL;
    }

    if (prtp_enc_th->packet_count > MAX_FU_A_RTP_COUNT)
    {
        elog("ph264th->data_count is error:[%d] ", prtp_enc_th->packet_count);
        return NULL;
    }

    for (i = 0; i < MAX_FU_A_RTP_COUNT; i++)
    {
        if (prtp_enc_th->packets[i].used_flag)
        {
            prtp_enc_th->packets[i].used_flag = 0;
            prtp_enc_th->packet_count -= 1;
            return &(prtp_enc_th->packets[i]);
        }
    }

    return NULL;
}

static VIDEO_RTP_DEC_TH alloc_h265_rtp_dec_th(uint32_t h265_donl_flag)
{
    H264_RTP_DEC_ST *pth = (H264_RTP_DEC_ST *)alloc_h264_rtp_dec_th();
    if (NULL == pth)
    {
        return NULL;
    }

    pth->codec_type = VIDEO_CODEC_H265;
    pth->donl_flag = h265_donl_flag;

    return pth;
}


static void free_h265_rtp_dec_th(VIDEO_RTP_DEC_TH  pdec_th)
{
    free_h264_rtp_dec_th(pdec_th);
}

static int32_t single_packet_rtp_dec(H264_RTP_DEC_ST *prtp_dec_th, uint8_t nal_type, uint8_t pre_mark_flg,
            uint8_t cur_mark_flg, uint32_t  pre_timestamp, uint32_t timestamp, uint8_t *pnalbuf, int32_t nal_len)
{
    if (nal_len < 1)
    {
        elog("nal len[%d] is less than 1.", nal_len);
        return -1;
    }

    //单包模式：
    //只有视频帧的时候，把mark标志作为判断是否结束分包
    if (nal_type < 32)
    {
        if (pre_mark_flg && cur_mark_flg)  //上一个标志和当前标志都为1，说明是单个包
        {
            reset_h264_rtp_dec_th_data(prtp_dec_th);
            h264_rtp_dec_add_one_nalu_one_frame(prtp_dec_th, nal_type, timestamp, nal_len, pnalbuf);
        }
        else if (pre_mark_flg && !cur_mark_flg)  //上一个包标志为1，当前为0，说明是新的帧开始
        {
            reset_h264_rtp_dec_th_data(prtp_dec_th);
            h264_rtp_dec_add_nalu_start_sequence(prtp_dec_th, 1, nal_type, timestamp);
            h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, nal_len, pnalbuf);
        }
        else if (!pre_mark_flg && !cur_mark_flg) //上一个标志和当前标志都为0，说明是一帧的中间包
        {
            //这里要判断时戳。两个时戳不同，则是新的帧，发生了丢包可能
            if (pre_timestamp == timestamp) //时戳相同，则是同一帧的中间包。
            {
                //添加nal 头：00 00 01
                h264_rtp_dec_add_nalu_start_sequence(prtp_dec_th, 0, nal_type, timestamp);
                h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, nal_len, pnalbuf);
            }
            else //可能发生了丢包，最后一个带有mark的包没有了，开始新的包
            {
                //首先将上一个包结束
                h264_rtp_dec_add_frame_end_flag(prtp_dec_th);

                //开始一个新的包
                h264_rtp_dec_add_nalu_start_sequence(prtp_dec_th, 1, nal_type, timestamp);
                h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, nal_len, pnalbuf);
            }
        }
        else if (!pre_mark_flg && cur_mark_flg) //上一个标志0， 当前标志为1，说明是一帧结束包
        {
            //这里要判断时戳。两个时戳不同，怎么是新的帧，发生了丢包可能
            if (pre_timestamp == timestamp) //时戳相同，则是同一帧的结束包。
            {
                //添加nal 头：00 00 01
                h264_rtp_dec_add_nalu_start_sequence(prtp_dec_th, 0, nal_type, timestamp);
                h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, nal_len, pnalbuf);
                h264_rtp_dec_add_frame_end_flag(prtp_dec_th);
            }
            else //可能发生了丢包，最后一个带有mark的包没有了，开始新的包
            {
                //首先将上一个包结束
                h264_rtp_dec_add_frame_end_flag(prtp_dec_th);

                //开始一个新的包
                h264_rtp_dec_add_one_nalu_one_frame(prtp_dec_th, nal_type, timestamp, nal_len, pnalbuf);
            }
        }
    }
    else
    {
        //其他类型的包，不管类型，都认为是新的包。并把上一个包mark标志置为1,表示下一个包是新的开始
        //首先将上一个包结束
        h264_rtp_dec_add_frame_end_flag(prtp_dec_th);

        h264_rtp_dec_add_one_nalu_one_frame(prtp_dec_th, nal_type, timestamp, nal_len, pnalbuf);
        prtp_dec_th->pre_mark_flg = 1;
    }

    return 0;
}

static int32_t stap_packet_rtp_dec(H264_RTP_DEC_ST *prtp_dec_th, uint8_t pre_mark_flg,
                    uint8_t cur_mark_flg, uint32_t timestamp, uint8_t *pnalbuf, int32_t nal_len)
{
    if (nal_len < 3)
    {
        elog("nal len[%d] is less than 3.", nal_len);
        return -1;
    }

    /* pass the HEVC payload header */
    pnalbuf += RTP_HEVC_PAYLOAD_HEADER_SIZE;
    nal_len -= RTP_HEVC_PAYLOAD_HEADER_SIZE;

    /* pass the HEVC DONL field */
    if (prtp_dec_th->donl_flag)
    {
        pnalbuf += RTP_HEVC_DONL_FIELD_SIZE;
        nal_len -= RTP_HEVC_DONL_FIELD_SIZE;
    }

    reset_h264_rtp_dec_th_data(prtp_dec_th);

    while (nal_len > 2)
    {
        uint16_t nal_size = pnalbuf[0] * 256 + pnalbuf[1];
        // sikp the size :two byte
        pnalbuf     += 2;
        nal_len -= 2;

        if (nal_size <= nal_len)
        {
            h264_rtp_dec_add_one_nalu_one_frame(prtp_dec_th, (pnalbuf[0] >> 1) & 0x3f, timestamp, nal_size, pnalbuf);   //这里的type获取要看每个nal 的

            pnalbuf += nal_size;
            nal_len -= nal_size;
        }
        else
        {
            elog("nal size exceeds length: %d %d.", nal_size, nal_len);
            break;  //break while
        }
    }

    return 0;
}

static int32_t fu_packet_rtp_dec(H264_RTP_DEC_ST *prtp_dec_th, uint8_t pre_mark_flg, uint8_t cur_mark_flg,
            uint32_t  pre_timestamp, uint32_t timestamp, uint8_t *pnalbuf, int32_t nal_len, uint32_t  is_loss_flg)
{
    int32_t first_fragment = 0;
    int32_t last_fragment = 0;
    int32_t fu_type = 0;
    uint8_t pl_header[2] = "";

    if (nal_len < RTP_HEVC_PAYLOAD_HEADER_SIZE + 1)
    {
        elog("nal len[%d] is less than 3.", nal_len);
        return -1;
    }

    pl_header[0] = pnalbuf[0];
    pl_header[1] = pnalbuf[1];

    /* pass the HEVC payload header */
    pnalbuf += RTP_HEVC_PAYLOAD_HEADER_SIZE;
    nal_len -= RTP_HEVC_PAYLOAD_HEADER_SIZE;

    /*
     *    decode the FU header
     *
     *     0 1 2 3 4 5 6 7
     *    +-+-+-+-+-+-+-+-+
     *    |S|E|  FuType   |
     *    +---------------+
     *
     *       Start fragment (S): 1 bit
     *       End fragment (E): 1 bit
     *       FuType: 6 bits
     */
    first_fragment = pnalbuf[0] & 0x80;
    last_fragment  = pnalbuf[0] & 0x40;
    fu_type        = pnalbuf[0] & 0x3f;

    /* pass the HEVC FU header */
    pnalbuf += RTP_HEVC_FU_HEADER_SIZE;
    nal_len -= RTP_HEVC_FU_HEADER_SIZE;

    /* pass the HEVC DONL field */
    if (prtp_dec_th->donl_flag)
    {
        pnalbuf += RTP_HEVC_DONL_FIELD_SIZE;
        nal_len -= RTP_HEVC_DONL_FIELD_SIZE;
    }

    if (first_fragment && last_fragment)
    {
        elog("Illegal combination of S and E bit in RTP/HEVC packet");
        return -1;
    }

    //要考虑的一种特殊情况是，一帧中有多个slice都是用fu_a编码的。最后要输出为一个nalu，而不是多个
    if (nal_len > 1)
    {
        if (first_fragment)//起始包
        {
            //首先添加包头：先前两个字节利用起来
            *(pnalbuf - 2) = (pl_header[0] & 0x81) | (fu_type << 1);
            *(pnalbuf - 1)  = pl_header[1];

            if (pre_mark_flg)   //上一帧为结束帧，这一帧为新帧
            {
                reset_h264_rtp_dec_th_data(prtp_dec_th);
                h264_rtp_dec_add_nalu_start_sequence(prtp_dec_th, 1, fu_type, timestamp);
            }
            else if (pre_timestamp != timestamp)//上一阵mark为0，但是这一帧时戳不同了，要停止上一帧，开始新帧
            {
                h264_rtp_dec_add_frame_end_flag(prtp_dec_th);
                h264_rtp_dec_add_nalu_start_sequence(prtp_dec_th, 1, fu_type, timestamp);
            }
            else    //否则，就是中间的帧了
            {
                h264_rtp_dec_add_nalu_start_sequence(prtp_dec_th, 0, fu_type, timestamp);
            }
            h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, nal_len + 2, pnalbuf - 2); //把前面的包头加入进来
        }
        else if (!last_fragment) //中间包
        {
            if (pre_timestamp != timestamp)//上一阵mark为0，但是这一帧时戳不同了，要停止上一帧，开始新帧
            {
                h264_rtp_dec_add_frame_end_flag(prtp_dec_th);
            }

            if (is_loss_flg)
            {
                prtp_dec_th->fua_loss_flg = 1;
            }

            if (prtp_dec_th->fua_loss_flg)
            {
//                dlog("The Fua loss flg is True, so lose this packet!");
//                return prtp_dec_th->packet_count;
            }
            h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, nal_len, pnalbuf);
        }
        else if (last_fragment)//结束包
        {
            if (pre_timestamp != timestamp)//上一阵mark为0，但是这一帧时戳不同了，要停止上一帧，开始新帧
            {
                h264_rtp_dec_add_frame_end_flag(prtp_dec_th);
            }
            if (is_loss_flg)
            {
                prtp_dec_th->fua_loss_flg = 1;
            }
            if (prtp_dec_th->fua_loss_flg)
            {
//                dlog("The Fua loss flg is True, so lose this packet!");
//                return prtp_dec_th->packet_count;
            }
            h264_rtp_dec_add_part_data_of_frame(prtp_dec_th, nal_len, pnalbuf);
            if (cur_mark_flg)
            {
                h264_rtp_dec_add_frame_end_flag(prtp_dec_th);
            }
        }
    }
    else
    {
        elog("Too short data for FU-A H264 RTP packet");
        return -1;
    }
    return 0;
}

//rtp_buf:包含rtp包头.返回值：获得包的个数.-1表示解析失败
static int h265_dec_rtp(VIDEO_RTP_DEC_TH  pdec_th, uint8_t *rtp_buf, int buf_len)
{

    uint8_t * pnalbuf = NULL;
    int32_t tid = 0;
    int32_t lid = 0;

    uint8_t cur_mark_flg = 0;   //当前包mark标志
    uint8_t pre_mark_flg = 0;   //上一个包的mark标志
    int32_t nal_len = 0;
    uint8_t nal_type = 0;
    H264_RTP_DEC_ST *prtp_dec_th = (H264_RTP_DEC_ST *)pdec_th;
    uint16_t  seq = 0;
    uint32_t  pre_timestamp = 0;
    uint32_t  timestamp = 0;
    uint32_t  is_loss_flg = 0;  //是否发生过丢包

    if ((NULL == pdec_th) || (NULL == rtp_buf))
    {
        return -1;
    }

    if (buf_len < RTP_HEADER_LEN + RTP_HEVC_PAYLOAD_HEADER_SIZE + 1)
    {
        elog("h265 rtp dec: buf len[%d] is less than 15", buf_len);
        return -1;
    }

    //获取序号和时戳
    seq = ntohs(*(uint16_t *)(&rtp_buf[2]));
    timestamp = ntohl(*(uint32_t *)(&rtp_buf[4]));
    if ((prtp_dec_th->pre_seq + 1 != seq) && (prtp_dec_th->pre_seq != -1))   //有跳跃且非初始值；
    {
        if ((prtp_dec_th->pre_seq == 65535) && (seq == 0))  //临界值，则不算丢包
        {
        }
        else
        {
            wlog("There is lose packet or sequence is error.pre[%d], cur[%d]", prtp_dec_th->pre_seq, seq);
            is_loss_flg = 1;
        }
    }
    prtp_dec_th->pre_seq = seq; //记录当前序号
    pre_timestamp = prtp_dec_th->pre_timestamp; //记录上一个时戳
    prtp_dec_th->pre_timestamp = timestamp;

    pnalbuf = rtp_buf + RTP_HEADER_LEN;//首先跳过12个字节的rtp包头。
    nal_len = buf_len - RTP_HEADER_LEN;

    /*
     * decode the HEVC payload header according to section 4 of draft version 6:
     *
     *    0                   1
     *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
     *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *   |F|   Type    |  LayerId  | TID |
     *   +-------------+-----------------+
     *
     *      Forbidden zero (F): 1 bit
     *      NAL unit type (Type): 6 bits
     *      NUH layer ID (LayerId): 6 bits
     *      NUH temporal ID plus 1 (TID): 3 bits
     */
    nal_type =  (pnalbuf[0] >> 1) & 0x3f;
    lid  = ((pnalbuf[0] << 5) & 0x20) | ((pnalbuf[1] >> 3) & 0x1f);
    tid  =   pnalbuf[1] & 0x07;

    /* sanity check for correct layer ID */
    if (lid) {
        /* future scalable or 3D video coding extensions */
        elog("Multi-layer HEVC coding");
        return -1;
    }

    /* sanity check for correct temporal ID */
    if (!tid) {
        elog("Illegal temporal ID in RTP/HEVC packet");
        return -1;
    }

    /* sanity check for correct NAL unit type */
    if (nal_type > 50) {
        elog("Unsupported (HEVC) NAL type (%d)", nal_type);
        return -1;
    }

    cur_mark_flg = rtp_buf[1] & 0x80;    //取第一位的值，为mark标志位
    pre_mark_flg = prtp_dec_th->pre_mark_flg;
    prtp_dec_th->pre_mark_flg = cur_mark_flg;   //更改对象记录

    switch (nal_type)
    {
        /* video parameter set (VPS) */
        case 32:
        /* sequence parameter set (SPS) */
        case 33:
        /* picture parameter set (PPS) */
        case 34:
        /*  supplemental enhancement information (SEI) */
        case 39:
        /* single NAL unit packet */
        default:
            single_packet_rtp_dec(prtp_dec_th, nal_type, pre_mark_flg, cur_mark_flg, pre_timestamp, timestamp, pnalbuf, nal_len);
            return prtp_dec_th->packet_count;
            break;

        /* aggregated packet (AP) - with two or more NAL units */
        case 48:
            stap_packet_rtp_dec(prtp_dec_th, pre_mark_flg, cur_mark_flg, timestamp, pnalbuf, nal_len);
            return prtp_dec_th->packet_count;
            break;
        /* fragmentation unit (FU) */
        case 49:
            fu_packet_rtp_dec(prtp_dec_th, pre_mark_flg, cur_mark_flg, pre_timestamp, timestamp, pnalbuf, nal_len, is_loss_flg);
            return prtp_dec_th->packet_count;
            break;
        /* PACI packet */
        case 50:
            /* Temporal scalability control information (TSCI) */
            dlog( "PACI packets for RTP/HEVC");
            return 0;
            break;
    }

    return -1;

}

//函数返回数据的指针，没有数据则返回空。这个函数要一直调用，直到返回null为止，否则数据会被覆盖
//返回的指针或指针中的缓冲区不可以作为上层的存储缓冲区，如果需要保存一段时间，则需要重新申请新的缓冲区并拷贝。
//used_flag字段返回前会被置为0.
static H264_NAL_DATA * h265_dec_rtp_get_nal_data(VIDEO_RTP_DEC_TH  pdec_th)
{
    return h264_dec_rtp_get_nal_data(pdec_th);
}



VIDEO_RTP_DEC_TH alloc_video_rtp_dec_th(VIDEO_DEC_RTP_PARAMETER *ppara)
{
    if (NULL == ppara)
    {
        elog("input error!");
        return NULL;
    }

    if (ppara->vtype == VIDEO_RTP_H264)
    {
        return alloc_h264_rtp_dec_th();
    }
    else
    {
        return alloc_h265_rtp_dec_th(ppara->donl_flag);
    }
}

void free_video_rtp_dec_th(VIDEO_RTP_DEC_TH  pdec_th)
{
    H264_RTP_DEC_ST *ptmp_th = NULL;

    if (NULL == pdec_th)
    {
        elog("input is error!");
        return ;
    }

    ptmp_th = pdec_th;
    if (ptmp_th->codec_type == VIDEO_CODEC_H264)
    {
        free_h264_rtp_dec_th(pdec_th);
    }
    else
    {
        free_h265_rtp_dec_th(pdec_th);
    }
}

//rtp_buf:包含rtp包头.返回值：获得包的个数.-1表示解析失败
int video_dec_rtp(VIDEO_RTP_DEC_TH  pdec_th, uint8_t *rtp_buf, int buf_len)
{
    H264_RTP_DEC_ST *ptmp_th = NULL;

    if (NULL == pdec_th)
    {
        elog("input is error!");
        return -1;
    }

    ptmp_th = pdec_th;
    if (ptmp_th->codec_type == VIDEO_CODEC_H264)
    {
        return h264_dec_rtp(pdec_th, rtp_buf, buf_len);
    }
    else
    {
        return h265_dec_rtp(pdec_th, rtp_buf, buf_len);
    }

}

//函数返回数据的指针，没有数据则返回空。这个函数要一直调用，直到返回null为止，否则数据会被覆盖
//返回的指针或指针中的缓冲区不可以作为上层的存储缓冲区，如果需要保存一段时间，则需要重新申请新的缓冲区并拷贝。
//used_flag字段返回前会被置为0.
H264_NAL_DATA * video_dec_rtp_get_nal_data(VIDEO_RTP_DEC_TH  pdec_th)
{
    H264_RTP_DEC_ST *ptmp_th = NULL;

    if (NULL == pdec_th)
    {
        elog("input is error!");
        return NULL;
    }

    ptmp_th = pdec_th;
    if (ptmp_th->codec_type == VIDEO_CODEC_H264)
    {
        return h264_dec_rtp_get_nal_data(pdec_th);
    }
    else
    {
        return h265_dec_rtp_get_nal_data(pdec_th);
    }
}


































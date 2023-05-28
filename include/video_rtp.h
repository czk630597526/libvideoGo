/*
 * video_rtp.h
 *  整合了h264和h265 的rtp的编解码。统一成一样的接口。
 *  暂时没有编码。
 */

#ifndef VIDEO_RTP_H_
#define VIDEO_RTP_H_


typedef void * VIDEO_RTP_DEC_TH;

typedef enum __VIDEO_RTP_TYPE
{
    VIDEO_RTP_H264 = 0,
    VIDEO_RTP_H265 = 1,

}VIDEO_RTP_TYPE;

typedef struct __VIDEO_DEC_RTP_PARAMETER
{
    VIDEO_RTP_TYPE vtype;   //视频类型
    uint32_t donl_flag;     //donl_flag:using_donl_field：h265时有用。
                            //如果存在这一项，则在rtp头或者fu头后面会存在两个字节的donl字段，暂时不知道具体的含义。
                            //它是根据SDP来确定是否存在的：sprop-max-don-diff或sprop-depack-buf-nalus的值大于0.
                            //默认不使用。
}VIDEO_DEC_RTP_PARAMETER;

#ifdef __cplusplus
extern "C" {
#endif

//donl_flag:using_donl_field：h265时有用。
//如果存在这一项，则在rtp头或者fu头后面会存在两个字节的donl字段，暂时不知道具体的含义。
//它是根据SDP来确定是否存在的：sprop-max-don-diff或sprop-depack-buf-nalus的值大于0.
//默认不使用。
VIDEO_RTP_DEC_TH alloc_video_rtp_dec_th(VIDEO_DEC_RTP_PARAMETER *ppara);

void free_video_rtp_dec_th(VIDEO_RTP_DEC_TH  pdec_th);
//rtp_buf:包含rtp包头.返回值：获得包的个数.-1表示解析失败
int video_dec_rtp(VIDEO_RTP_DEC_TH  pdec_th, uint8_t *rtp_buf, int buf_len);

//函数返回数据的指针，没有数据则返回空。这个函数要一直调用，直到返回null为止，否则数据会被覆盖
//返回的指针或指针中的缓冲区不可以作为上层的存储缓冲区，如果需要保存一段时间，则需要重新申请新的缓冲区并拷贝。
//used_flag字段返回前会被置为0.
H264_NAL_DATA * video_dec_rtp_get_nal_data(VIDEO_RTP_DEC_TH  pdec_th);

#ifdef __cplusplus
}
#endif


#endif /* VIDEO_RTP_H_ */

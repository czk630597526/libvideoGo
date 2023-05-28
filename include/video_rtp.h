/*
 * video_rtp.h
 *  ������h264��h265 ��rtp�ı���롣ͳһ��һ���Ľӿڡ�
 *  ��ʱû�б��롣
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
    VIDEO_RTP_TYPE vtype;   //��Ƶ����
    uint32_t donl_flag;     //donl_flag:using_donl_field��h265ʱ���á�
                            //���������һ�����rtpͷ����fuͷ�������������ֽڵ�donl�ֶΣ���ʱ��֪������ĺ��塣
                            //���Ǹ���SDP��ȷ���Ƿ���ڵģ�sprop-max-don-diff��sprop-depack-buf-nalus��ֵ����0.
                            //Ĭ�ϲ�ʹ�á�
}VIDEO_DEC_RTP_PARAMETER;

#ifdef __cplusplus
extern "C" {
#endif

//donl_flag:using_donl_field��h265ʱ���á�
//���������һ�����rtpͷ����fuͷ�������������ֽڵ�donl�ֶΣ���ʱ��֪������ĺ��塣
//���Ǹ���SDP��ȷ���Ƿ���ڵģ�sprop-max-don-diff��sprop-depack-buf-nalus��ֵ����0.
//Ĭ�ϲ�ʹ�á�
VIDEO_RTP_DEC_TH alloc_video_rtp_dec_th(VIDEO_DEC_RTP_PARAMETER *ppara);

void free_video_rtp_dec_th(VIDEO_RTP_DEC_TH  pdec_th);
//rtp_buf:����rtp��ͷ.����ֵ����ð��ĸ���.-1��ʾ����ʧ��
int video_dec_rtp(VIDEO_RTP_DEC_TH  pdec_th, uint8_t *rtp_buf, int buf_len);

//�����������ݵ�ָ�룬û�������򷵻ؿա��������Ҫһֱ���ã�ֱ������nullΪֹ���������ݻᱻ����
//���ص�ָ���ָ���еĻ�������������Ϊ�ϲ�Ĵ洢�������������Ҫ����һ��ʱ�䣬����Ҫ���������µĻ�������������
//used_flag�ֶη���ǰ�ᱻ��Ϊ0.
H264_NAL_DATA * video_dec_rtp_get_nal_data(VIDEO_RTP_DEC_TH  pdec_th);

#ifdef __cplusplus
}
#endif


#endif /* VIDEO_RTP_H_ */

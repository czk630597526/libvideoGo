/*
 * h264_ps.h
 *
 */

#ifndef H264_PS_H_
#define H264_PS_H_

#include "libv_log.h"
#include "libvideo.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SAR_Extended      255        // Extended_SAR

/**
   Sequence Parameter Set
   @see 7.3.2.1 Sequence parameter set RBSP syntax
   @see read_seq_parameter_set_rbsp
   @see write_seq_parameter_set_rbsp
   @see debug_sps
*/
typedef struct
{
    int profile_idc;
    int constraint_set0_flag;
    int constraint_set1_flag;
    int constraint_set2_flag;
    int constraint_set3_flag;
    int constraint_set4_flag;
    int constraint_set5_flag;
    int reserved_zero_2bits;
    int level_idc;
    int seq_parameter_set_id;
    int chroma_format_idc;
    int residual_colour_transform_flag;
    int bit_depth_luma_minus8;
    int bit_depth_chroma_minus8;
    int qpprime_y_zero_transform_bypass_flag;
    int seq_scaling_matrix_present_flag;
      int seq_scaling_list_present_flag[8];
      int* ScalingList4x4[6];
      int UseDefaultScalingMatrix4x4Flag[6];
      int* ScalingList8x8[2];
      int UseDefaultScalingMatrix8x8Flag[2];
    int log2_max_frame_num_minus4;
    int pic_order_cnt_type;
      int log2_max_pic_order_cnt_lsb_minus4;
      int delta_pic_order_always_zero_flag;
      int offset_for_non_ref_pic;
      int offset_for_top_to_bottom_field;
      int num_ref_frames_in_pic_order_cnt_cycle;
      int offset_for_ref_frame[256];
    int num_ref_frames;
    int gaps_in_frame_num_value_allowed_flag;
    int pic_width_in_mbs_minus1;
    int pic_height_in_map_units_minus1;
    int frame_mbs_only_flag;
    int mb_adaptive_frame_field_flag;
    int direct_8x8_inference_flag;
    int frame_cropping_flag;
      int frame_crop_left_offset;
      int frame_crop_right_offset;
      int frame_crop_top_offset;
      int frame_crop_bottom_offset;
    int vui_parameters_present_flag;

    struct
    {
        int aspect_ratio_info_present_flag;
          int aspect_ratio_idc;
            int sar_width;
            int sar_height;
        int overscan_info_present_flag;
          int overscan_appropriate_flag;
        int video_signal_type_present_flag;
          int video_format;
          int video_full_range_flag;
          int colour_description_present_flag;
            int colour_primaries;
            int transfer_characteristics;
            int matrix_coefficients;
        int chroma_loc_info_present_flag;
          int chroma_sample_loc_type_top_field;
          int chroma_sample_loc_type_bottom_field;
        int timing_info_present_flag;
          int num_units_in_tick;
          int time_scale;
          int fixed_frame_rate_flag;
        int nal_hrd_parameters_present_flag;
        int vcl_hrd_parameters_present_flag;
          int low_delay_hrd_flag;
        int pic_struct_present_flag;
        int bitstream_restriction_flag;
          int motion_vectors_over_pic_boundaries_flag;
          int max_bytes_per_pic_denom;
          int max_bits_per_mb_denom;
          int log2_max_mv_length_horizontal;
          int log2_max_mv_length_vertical;
          int num_reorder_frames;
          int max_dec_frame_buffering;
    } vui;

    struct
    {
        int cpb_cnt_minus1;
        int bit_rate_scale;
        int cpb_size_scale;
          int bit_rate_value_minus1[32]; // up to cpb_cnt_minus1, which is <= 31
          int cpb_size_value_minus1[32];
          int cbr_flag[32];
        int initial_cpb_removal_delay_length_minus1;
        int cpb_removal_delay_length_minus1;
        int dpb_output_delay_length_minus1;
        int time_offset_length;
    } hrd;

} sps_t;


/**
   Picture Parameter Set
   @see 7.3.2.2 Picture parameter set RBSP syntax
   @see read_pic_parameter_set_rbsp
   @see write_pic_parameter_set_rbsp
   @see debug_pps
*/
typedef struct
{
    int pic_parameter_set_id;
    int seq_parameter_set_id;
    int entropy_coding_mode_flag;
    int pic_order_present_flag;
    int num_slice_groups_minus1;
    int slice_group_map_type;
      int run_length_minus1[8]; // up to num_slice_groups_minus1, which is <= 7 in Baseline and Extended, 0 otheriwse
      int top_left[8];
      int bottom_right[8];
      int slice_group_change_direction_flag;
      int slice_group_change_rate_minus1;
      int pic_size_in_map_units_minus1;
      int slice_group_id[256]; // FIXME what size?
    int num_ref_idx_l0_active_minus1;
    int num_ref_idx_l1_active_minus1;
    int weighted_pred_flag;
    int weighted_bipred_idc;
    int pic_init_qp_minus26;
    int pic_init_qs_minus26;
    int chroma_qp_index_offset;
    int deblocking_filter_control_present_flag;
    int constrained_intra_pred_flag;
    int redundant_pic_cnt_present_flag;

    // set iff we carry any of the optional headers
    int _more_rbsp_data_present;

    int transform_8x8_mode_flag;
    int pic_scaling_matrix_present_flag;
       int pic_scaling_list_present_flag[8];
       int* ScalingList4x4[6];
       int UseDefaultScalingMatrix4x4Flag[6];
       int* ScalingList8x8[2];
       int UseDefaultScalingMatrix8x8Flag[2];
    int second_chroma_qp_index_offset;
} pps_t;

//return 0表示成功，-1表示失败。sps_buf为sps_buf为sps数据(不包括开始码)，buf_size为数据大小。sps为输出
int dec_seq_parameter_set_rbsp(sps_t* sps, uint8_t* sps_buf, int buf_size);
//return 0表示成功，-1表示失败。pps_buf为pps数据，buf_size为数据大小。pps为输出
int dec_pic_parameter_set_rbsp(pps_t* pps, uint8_t* pps_buf, int buf_size);
//return sps的长度，-1表示失败。sps_buf为sps输出数据(不包括开始码)，buf_size为数据大小。sps_buf为输出
int enc_seq_parameter_set_rbsp(sps_t* sps, uint8_t* sps_buf, int buf_size);
//return pps的长度，-1表示失败。pps_buf为pps输出数据(不包括开始码)，buf_size为数据大小。pps_buf为输出
int enc_pic_parameter_set_rbsp(pps_t* pps, uint8_t* pps_buf, int buf_size);

void debug_sps(sps_t* sps);
void debug_pps(pps_t* pps);

#ifdef __cplusplus
}
#endif




#endif /* H264_PS_H_ */

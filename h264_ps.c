/*
 * h264_ps.c
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bs.h"
#include "h264_ps.h"
#include "libv_log.h"

#define MAX_SPS_BUF_LEN  1024
#define MAX_PPS_BUF_LEN  1024

static int intlog2(int x)
{
    int log = 0;
    if (x < 0) { x = 0; }
    while ((x >> log) > 0)
    {
        log++;
    }
    if (log > 0 && x == 1<<(log-1)) { log--; }
    return log;
}
//7.3.2.1.1 Scaling list syntax
static void read_scaling_list(bs_t* b, int* scalingList, int sizeOfScalingList, int useDefaultScalingMatrixFlag )
{
    int j;
    if(scalingList == NULL)
    {
        return;
    }

    int lastScale = 8;
    int nextScale = 8;
    for( j = 0; j < sizeOfScalingList; j++ )
    {
        if( nextScale != 0 )
        {
            int delta_scale = bs_read_se(b);
            nextScale = ( lastScale + delta_scale + 256 ) % 256;
            useDefaultScalingMatrixFlag = ( j == 0 && nextScale == 0 );
        }
        scalingList[ j ] = ( nextScale == 0 ) ? lastScale : nextScale;
        lastScale = scalingList[ j ];
    }
}

//Appendix E.1.2 HRD parameters syntax
static void read_hrd_parameters(sps_t* sps, bs_t* b)
{
    int SchedSelIdx;

    sps->hrd.cpb_cnt_minus1 = bs_read_ue(b);
    sps->hrd.bit_rate_scale = bs_read_u(b,4);
    sps->hrd.cpb_size_scale = bs_read_u(b,4);
    for( SchedSelIdx = 0; SchedSelIdx <= sps->hrd.cpb_cnt_minus1; SchedSelIdx++ )
    {
        sps->hrd.bit_rate_value_minus1[ SchedSelIdx ] = bs_read_ue(b);
        sps->hrd.cpb_size_value_minus1[ SchedSelIdx ] = bs_read_ue(b);
        sps->hrd.cbr_flag[ SchedSelIdx ] = bs_read_u1(b);
    }
    sps->hrd.initial_cpb_removal_delay_length_minus1 = bs_read_u(b,5);
    sps->hrd.cpb_removal_delay_length_minus1 = bs_read_u(b,5);
    sps->hrd.dpb_output_delay_length_minus1 = bs_read_u(b,5);
    sps->hrd.time_offset_length = bs_read_u(b,5);
}

//Appendix E.1.1 VUI parameters syntax
static void read_vui_parameters(sps_t* sps, bs_t* b)
{
    sps->vui.aspect_ratio_info_present_flag = bs_read_u1(b);
    if( sps->vui.aspect_ratio_info_present_flag )
    {
        sps->vui.aspect_ratio_idc = bs_read_u8(b);
        if( sps->vui.aspect_ratio_idc == SAR_Extended )
        {
            sps->vui.sar_width = bs_read_u(b,16);
            sps->vui.sar_height = bs_read_u(b,16);
        }
    }
    sps->vui.overscan_info_present_flag = bs_read_u1(b);
    if( sps->vui.overscan_info_present_flag )
    {
        sps->vui.overscan_appropriate_flag = bs_read_u1(b);
    }
    sps->vui.video_signal_type_present_flag = bs_read_u1(b);
    if( sps->vui.video_signal_type_present_flag )
    {
        sps->vui.video_format = bs_read_u(b,3);
        sps->vui.video_full_range_flag = bs_read_u1(b);
        sps->vui.colour_description_present_flag = bs_read_u1(b);
        if( sps->vui.colour_description_present_flag )
        {
            sps->vui.colour_primaries = bs_read_u8(b);
            sps->vui.transfer_characteristics = bs_read_u8(b);
            sps->vui.matrix_coefficients = bs_read_u8(b);
        }
    }
    sps->vui.chroma_loc_info_present_flag = bs_read_u1(b);
    if( sps->vui.chroma_loc_info_present_flag )
    {
        sps->vui.chroma_sample_loc_type_top_field = bs_read_ue(b);
        sps->vui.chroma_sample_loc_type_bottom_field = bs_read_ue(b);
    }
    sps->vui.timing_info_present_flag = bs_read_u1(b);
    if( sps->vui.timing_info_present_flag )
    {
        sps->vui.num_units_in_tick = bs_read_u(b,32);
        sps->vui.time_scale = bs_read_u(b,32);
        sps->vui.fixed_frame_rate_flag = bs_read_u1(b);
    }
    sps->vui.nal_hrd_parameters_present_flag = bs_read_u1(b);
    if( sps->vui.nal_hrd_parameters_present_flag )
    {
        read_hrd_parameters(sps, b);
    }
    sps->vui.vcl_hrd_parameters_present_flag = bs_read_u1(b);
    if( sps->vui.vcl_hrd_parameters_present_flag )
    {
        read_hrd_parameters(sps, b);
    }
    if( sps->vui.nal_hrd_parameters_present_flag || sps->vui.vcl_hrd_parameters_present_flag )
    {
        sps->vui.low_delay_hrd_flag = bs_read_u1(b);
    }
    sps->vui.pic_struct_present_flag = bs_read_u1(b);
    sps->vui.bitstream_restriction_flag = bs_read_u1(b);
    if( sps->vui.bitstream_restriction_flag )
    {
        sps->vui.motion_vectors_over_pic_boundaries_flag = bs_read_u1(b);
        sps->vui.max_bytes_per_pic_denom = bs_read_ue(b);
        sps->vui.max_bits_per_mb_denom = bs_read_ue(b);
        sps->vui.log2_max_mv_length_horizontal = bs_read_ue(b);
        sps->vui.log2_max_mv_length_vertical = bs_read_ue(b);
        sps->vui.num_reorder_frames = bs_read_ue(b);
        sps->vui.max_dec_frame_buffering = bs_read_ue(b);
    }
}

//7.3.2.11 RBSP trailing bits syntax
static void read_rbsp_trailing_bits(bs_t* b)
{
//    int rbsp_stop_one_bit = bs_read_u1( b ); // equal to 1
    bs_read_u1( b ); // equal to 1

    while( !bs_byte_aligned(b) )
    {
//        int rbsp_alignment_zero_bit = bs_read_u1( b ); // equal to 0
        bs_read_u1( b ); // equal to 0
    }
}

//为nal时间添加起始码竞争检查：为了防止和编码中的数据出现冲突，如果检查到数据中出现连续两个字节0x00，则在其后添加一个0x03。
//nal_src_data不能包含起始码
//return:新的数据长度。
static int start_code_emulation_prevention_add(uint8_t *nal_src_data, uint32_t src_len, uint8_t *nal_dst_data, uint32_t dst_len)
{
    uint8_t *psrc = nal_src_data;
    uint8_t *psrc_end = nal_src_data + src_len;
    uint8_t *pdst = nal_dst_data;
    uint8_t *pdst_end = nal_dst_data + dst_len;
    int zero_count = 0;

    while (psrc < psrc_end)
    {
        if (zero_count == 2 && *psrc <= 3)
        {
            //add the code 03
            *pdst++    = 3;
            zero_count        = 0;
        }
        if (*psrc == 0)
        {
            ++ zero_count;
        } else {
            zero_count        = 0;
        }
        *pdst++ = *psrc++;
        if (pdst >= pdst_end)
        {
            elog("There is no longer dest data buf in start_code_emulation_prevention_add!");
            return -1;
        }
    }

//    int i =0;
//    printf("\n------------start_code_emulation_prevention_add---------\nold buf:\n");
//    for(i = 0; i < src_len;i++)
//    {
//        printf("%02x", nal_src_data[i]);
//    }
//    printf("\n");
//
//    printf("\n------------start_code_emulation_prevention_add---------\nnew buf:\n");
//    for(i = 0; i < pdst - nal_dst_data;i++)
//    {
//        printf("%02x", nal_dst_data[i]);
//    }
//    printf("\n");



    return pdst - nal_dst_data;

}

//为nal时间删除起始码竞争检查：为了防止和编码中的数据出现冲突，如果检查到数据中出现连续两个字节0x00，则在其后添加一个0x03。
//nal_data不能包含起始码
//return:新的数据长度。
static int start_code_emulation_prevention_rmv(uint8_t *nal_data, uint32_t len)
{
    uint8_t *psrc = nal_data;
    uint8_t *psrc_end = nal_data + len;
    uint8_t *pdst = nal_data;

//    int i =0;
//    printf("\n------------start_code_emulation_prevention_rmv---------\nold buf:\n");
//    for(i = 0; i < len;i++)
//    {
//        printf("%02x", nal_data[i]);
//    }
//    printf("\n");

    while (psrc < psrc_end)
    {
        if ((psrc + 4 < psrc_end) && (psrc[0] == 0) && (psrc[1] == 0) && (psrc[2] == 3) && (psrc[3] <= 3))
        {
            *pdst = *psrc;
            *(pdst + 1) = *(psrc + 1);
            *(pdst + 2) = *(psrc + 3);//跳过3
            pdst += 3;
            psrc += 4;
        }
        else
        {
            *pdst++ = *psrc++;
        }
    }

//    printf("\n------------start_code_emulation_prevention_rmv---------\nnew buf:\n");
//    for(i = 0; i < pdst - nal_data;i++)
//    {
//        printf("%02x", nal_data[i]);
//    }
//    printf("\n");


    return pdst - nal_data;

}
//7.3.2.1 Sequence parameter set RBSP syntax
//return 0表示成功，-1表示失败。sps_buf为sps_buf为sps数据，buf_size为数据大小。sps为输出
int dec_seq_parameter_set_rbsp(sps_t* sps, uint8_t* sps_buf, int buf_size)
{
    int i;
    bs_t* b = NULL;
    int new_size = 0;
    uint8_t tmp_sps_buf[MAX_SPS_BUF_LEN] = "";

    if ((NULL == sps) || (NULL == sps_buf) || (buf_size > sizeof(tmp_sps_buf)))
    {
        input_error
        return -1;
    }

    //首先要对sps数据库进行处理，消除起始码竞争
    memcpy(tmp_sps_buf, sps_buf, buf_size);
    new_size = start_code_emulation_prevention_rmv(tmp_sps_buf, buf_size);

    b = bs_new(tmp_sps_buf, new_size);
    if (NULL == b)
    {
        elog("Alloc bs_new is fail!");
        return -1;
    }
    // NOTE can't read directly into sps because seq_parameter_set_id not yet known and so sps is not selected

    int profile_idc = bs_read_u8(b);
    int constraint_set0_flag = bs_read_u1(b);
    int constraint_set1_flag = bs_read_u1(b);
    int constraint_set2_flag = bs_read_u1(b);
    int constraint_set3_flag = bs_read_u1(b);
    int constraint_set4_flag = bs_read_u1(b);
    int constraint_set5_flag = bs_read_u1(b);
    int reserved_zero_2bits  = bs_read_u(b,2);  /* all 0's */
    int level_idc = bs_read_u8(b);
    int seq_parameter_set_id = bs_read_ue(b);

    sps->chroma_format_idc = 1;

    sps->profile_idc = profile_idc; // bs_read_u8(b);
    sps->constraint_set0_flag = constraint_set0_flag;//bs_read_u1(b);
    sps->constraint_set1_flag = constraint_set1_flag;//bs_read_u1(b);
    sps->constraint_set2_flag = constraint_set2_flag;//bs_read_u1(b);
    sps->constraint_set3_flag = constraint_set3_flag;//bs_read_u1(b);
    sps->constraint_set4_flag = constraint_set4_flag;//bs_read_u1(b);
    sps->constraint_set5_flag = constraint_set5_flag;//bs_read_u1(b);
    sps->reserved_zero_2bits = reserved_zero_2bits;//bs_read_u(b,2);
    sps->level_idc = level_idc; //bs_read_u8(b);
    sps->seq_parameter_set_id = seq_parameter_set_id; // bs_read_ue(b);
    if( sps->profile_idc == 100 || sps->profile_idc == 110 ||
        sps->profile_idc == 122 || sps->profile_idc == 144 )
    {
        sps->chroma_format_idc = bs_read_ue(b);
        if( sps->chroma_format_idc == 3 )
        {
            sps->residual_colour_transform_flag = bs_read_u1(b);
        }
        sps->bit_depth_luma_minus8 = bs_read_ue(b);
        sps->bit_depth_chroma_minus8 = bs_read_ue(b);
        sps->qpprime_y_zero_transform_bypass_flag = bs_read_u1(b);
        sps->seq_scaling_matrix_present_flag = bs_read_u1(b);
        if( sps->seq_scaling_matrix_present_flag )
        {
            for( i = 0; i < 8; i++ )
            {
                sps->seq_scaling_list_present_flag[ i ] = bs_read_u1(b);
                if( sps->seq_scaling_list_present_flag[ i ] )
                {
                    if( i < 6 )
                    {
                        read_scaling_list( b, sps->ScalingList4x4[ i ], 16,
                                      sps->UseDefaultScalingMatrix4x4Flag[ i ]);
                    }
                    else
                    {
                        read_scaling_list( b, sps->ScalingList8x8[ i - 6 ], 64,
                                      sps->UseDefaultScalingMatrix8x8Flag[ i - 6 ] );
                    }
                }
            }
        }
    }
    sps->log2_max_frame_num_minus4 = bs_read_ue(b);
    sps->pic_order_cnt_type = bs_read_ue(b);
    if( sps->pic_order_cnt_type == 0 )
    {
        sps->log2_max_pic_order_cnt_lsb_minus4 = bs_read_ue(b);
    }
    else if( sps->pic_order_cnt_type == 1 )
    {
        sps->delta_pic_order_always_zero_flag = bs_read_u1(b);
        sps->offset_for_non_ref_pic = bs_read_se(b);
        sps->offset_for_top_to_bottom_field = bs_read_se(b);
        sps->num_ref_frames_in_pic_order_cnt_cycle = bs_read_ue(b);
        for( i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++ )
        {
            sps->offset_for_ref_frame[ i ] = bs_read_se(b);
        }
    }
    sps->num_ref_frames = bs_read_ue(b);
    sps->gaps_in_frame_num_value_allowed_flag = bs_read_u1(b);
    sps->pic_width_in_mbs_minus1 = bs_read_ue(b);
    sps->pic_height_in_map_units_minus1 = bs_read_ue(b);
    sps->frame_mbs_only_flag = bs_read_u1(b);
    if( !sps->frame_mbs_only_flag )
    {
        sps->mb_adaptive_frame_field_flag = bs_read_u1(b);
    }
    sps->direct_8x8_inference_flag = bs_read_u1(b);
    sps->frame_cropping_flag = bs_read_u1(b);
    if( sps->frame_cropping_flag )
    {
        sps->frame_crop_left_offset = bs_read_ue(b);
        sps->frame_crop_right_offset = bs_read_ue(b);
        sps->frame_crop_top_offset = bs_read_ue(b);
        sps->frame_crop_bottom_offset = bs_read_ue(b);
    }
    sps->vui_parameters_present_flag = bs_read_u1(b);
    if( sps->vui_parameters_present_flag )
    {
        read_vui_parameters(sps, b);
    }
    read_rbsp_trailing_bits(b);

    bs_free(b);

    return 0;
}

int more_rbsp_data(pps_t* pps, bs_t* b)
{
    if ( bs_eof(b) ) { return 0; }
    if ( bs_peek_u1(b) == 1 ) { return 0; } // if next bit is 1, we've reached the stop bit
    return 1;
}

//7.3.2.2 Picture parameter set RBSP syntax
//return 0表示成功，-1表示失败。pps_buf为pps数据，buf_size为数据大小。pps为输出
int dec_pic_parameter_set_rbsp(pps_t* pps, uint8_t* pps_buf, int buf_size)
{
    int pps_id ;
    int i;
    int i_group;
    int new_size = 0;
    uint8_t tmp_pps_buf[MAX_PPS_BUF_LEN] = "";

    bs_t* b = NULL;

    if ((NULL == pps) || (NULL == pps_buf) || (buf_size > sizeof(tmp_pps_buf)))
    {
        input_error
        return -1;
    }

    //首先要对sps数据库进行处理，消除起始码竞争
    memcpy(tmp_pps_buf, pps_buf, buf_size);
    new_size = start_code_emulation_prevention_rmv(tmp_pps_buf, buf_size);

    b = bs_new(tmp_pps_buf, new_size);
    if (NULL == b)
    {
        elog("Alloc bs_new is fail!");
        return -1;
    }

    pps_id = bs_read_ue(b);

    pps->pic_parameter_set_id = pps_id;
    pps->seq_parameter_set_id = bs_read_ue(b);
    pps->entropy_coding_mode_flag = bs_read_u1(b);
    pps->pic_order_present_flag = bs_read_u1(b);
    pps->num_slice_groups_minus1 = bs_read_ue(b);

    if( pps->num_slice_groups_minus1 > 0 )
    {
        pps->slice_group_map_type = bs_read_ue(b);
        if( pps->slice_group_map_type == 0 )
        {
            for( i_group = 0; i_group <= pps->num_slice_groups_minus1; i_group++ )
            {
                pps->run_length_minus1[ i_group ] = bs_read_ue(b);
            }
        }
        else if( pps->slice_group_map_type == 2 )
        {
            for( i_group = 0; i_group < pps->num_slice_groups_minus1; i_group++ )
            {
                pps->top_left[ i_group ] = bs_read_ue(b);
                pps->bottom_right[ i_group ] = bs_read_ue(b);
            }
        }
        else if( pps->slice_group_map_type == 3 ||
                 pps->slice_group_map_type == 4 ||
                 pps->slice_group_map_type == 5 )
        {
            pps->slice_group_change_direction_flag = bs_read_u1(b);
            pps->slice_group_change_rate_minus1 = bs_read_ue(b);
        }
        else if( pps->slice_group_map_type == 6 )
        {
            pps->pic_size_in_map_units_minus1 = bs_read_ue(b);
            for( i = 0; i <= pps->pic_size_in_map_units_minus1; i++ )
            {
                pps->slice_group_id[ i ] = bs_read_u(b, intlog2( pps->num_slice_groups_minus1 + 1 ) ); // was u(v)
            }
        }
    }
    pps->num_ref_idx_l0_active_minus1 = bs_read_ue(b);
    pps->num_ref_idx_l1_active_minus1 = bs_read_ue(b);
    pps->weighted_pred_flag = bs_read_u1(b);
    pps->weighted_bipred_idc = bs_read_u(b,2);
    pps->pic_init_qp_minus26 = bs_read_se(b);
    pps->pic_init_qs_minus26 = bs_read_se(b);
    pps->chroma_qp_index_offset = bs_read_se(b);
    pps->deblocking_filter_control_present_flag = bs_read_u1(b);
    pps->constrained_intra_pred_flag = bs_read_u1(b);
    pps->redundant_pic_cnt_present_flag = bs_read_u1(b);

    pps->_more_rbsp_data_present = more_rbsp_data(pps, b);
    if( pps->_more_rbsp_data_present )
    {
        pps->transform_8x8_mode_flag = bs_read_u1(b);
        pps->pic_scaling_matrix_present_flag = bs_read_u1(b);
        if( pps->pic_scaling_matrix_present_flag )
        {
            for( i = 0; i < 6 + 2* pps->transform_8x8_mode_flag; i++ )
            {
                pps->pic_scaling_list_present_flag[ i ] = bs_read_u1(b);
                if( pps->pic_scaling_list_present_flag[ i ] )
                {
                    if( i < 6 )
                    {
                        read_scaling_list( b, pps->ScalingList4x4[ i ], 16,
                                      pps->UseDefaultScalingMatrix4x4Flag[ i ] );
                    }
                    else
                    {
                        read_scaling_list( b, pps->ScalingList8x8[ i - 6 ], 64,
                                      pps->UseDefaultScalingMatrix8x8Flag[ i - 6 ] );
                    }
                }
            }
        }
        pps->second_chroma_qp_index_offset = bs_read_se(b);
    }
    read_rbsp_trailing_bits(b);

    bs_free(b);

    return 0;
}

//Appendix E.1.2 HRD parameters syntax
static void write_hrd_parameters(sps_t* sps, bs_t* b)
{
    int SchedSelIdx;

    bs_write_ue(b, sps->hrd.cpb_cnt_minus1);
    bs_write_u(b,4, sps->hrd.bit_rate_scale);
    bs_write_u(b,4, sps->hrd.cpb_size_scale);
    for( SchedSelIdx = 0; SchedSelIdx <= sps->hrd.cpb_cnt_minus1; SchedSelIdx++ )
    {
        bs_write_ue(b, sps->hrd.bit_rate_value_minus1[ SchedSelIdx ]);
        bs_write_ue(b, sps->hrd.cpb_size_value_minus1[ SchedSelIdx ]);
        bs_write_u1(b, sps->hrd.cbr_flag[ SchedSelIdx ]);
    }
    bs_write_u(b,5, sps->hrd.initial_cpb_removal_delay_length_minus1);
    bs_write_u(b,5, sps->hrd.cpb_removal_delay_length_minus1);
    bs_write_u(b,5, sps->hrd.dpb_output_delay_length_minus1);
    bs_write_u(b,5, sps->hrd.time_offset_length);
}


//Appendix E.1.1 VUI parameters syntax
static void write_vui_parameters(sps_t* sps, bs_t* b)
{
    bs_write_u1(b, sps->vui.aspect_ratio_info_present_flag);
    if( sps->vui.aspect_ratio_info_present_flag )
    {
        bs_write_u8(b, sps->vui.aspect_ratio_idc);
        if( sps->vui.aspect_ratio_idc == SAR_Extended )
        {
            bs_write_u(b,16, sps->vui.sar_width);
            bs_write_u(b,16, sps->vui.sar_height);
        }
    }
    bs_write_u1(b, sps->vui.overscan_info_present_flag);
    if( sps->vui.overscan_info_present_flag )
    {
        bs_write_u1(b, sps->vui.overscan_appropriate_flag);
    }
    bs_write_u1(b, sps->vui.video_signal_type_present_flag);
    if( sps->vui.video_signal_type_present_flag )
    {
        bs_write_u(b,3, sps->vui.video_format);
        bs_write_u1(b, sps->vui.video_full_range_flag);
        bs_write_u1(b, sps->vui.colour_description_present_flag);
        if( sps->vui.colour_description_present_flag )
        {
            bs_write_u8(b, sps->vui.colour_primaries);
            bs_write_u8(b, sps->vui.transfer_characteristics);
            bs_write_u8(b, sps->vui.matrix_coefficients);
        }
    }
    bs_write_u1(b, sps->vui.chroma_loc_info_present_flag);
    if( sps->vui.chroma_loc_info_present_flag )
    {
        bs_write_ue(b, sps->vui.chroma_sample_loc_type_top_field);
        bs_write_ue(b, sps->vui.chroma_sample_loc_type_bottom_field);
    }
    bs_write_u1(b, sps->vui.timing_info_present_flag);
    if( sps->vui.timing_info_present_flag )
    {
        bs_write_u(b,32, sps->vui.num_units_in_tick);
        bs_write_u(b,32, sps->vui.time_scale);
        bs_write_u1(b, sps->vui.fixed_frame_rate_flag);
    }
    bs_write_u1(b, sps->vui.nal_hrd_parameters_present_flag);
    if( sps->vui.nal_hrd_parameters_present_flag )
    {
        write_hrd_parameters(sps, b);
    }
    bs_write_u1(b, sps->vui.vcl_hrd_parameters_present_flag);
    if( sps->vui.vcl_hrd_parameters_present_flag )
    {
        write_hrd_parameters(sps, b);
    }
    if( sps->vui.nal_hrd_parameters_present_flag || sps->vui.vcl_hrd_parameters_present_flag )
    {
        bs_write_u1(b, sps->vui.low_delay_hrd_flag);
    }
    bs_write_u1(b, sps->vui.pic_struct_present_flag);
    bs_write_u1(b, sps->vui.bitstream_restriction_flag);
    if( sps->vui.bitstream_restriction_flag )
    {
        bs_write_u1(b, sps->vui.motion_vectors_over_pic_boundaries_flag);
        bs_write_ue(b, sps->vui.max_bytes_per_pic_denom);
        bs_write_ue(b, sps->vui.max_bits_per_mb_denom);
        bs_write_ue(b, sps->vui.log2_max_mv_length_horizontal);
        bs_write_ue(b, sps->vui.log2_max_mv_length_vertical);
        bs_write_ue(b, sps->vui.num_reorder_frames);
        bs_write_ue(b, sps->vui.max_dec_frame_buffering);
    }
}


//7.3.2.1.1 Scaling list syntax
static void write_scaling_list(bs_t* b, int* scalingList, int sizeOfScalingList, int useDefaultScalingMatrixFlag )
{
    int j;

    int lastScale = 8;
    int nextScale = 8;

    for( j = 0; j < sizeOfScalingList; j++ )
    {
        int delta_scale;

        if( nextScale != 0 )
        {
            // FIXME will not write in most compact way - could truncate list if all remaining elements are equal
            nextScale = scalingList[ j ];

            if (useDefaultScalingMatrixFlag)
            {
                nextScale = 0;
            }

            delta_scale = (nextScale - lastScale) % 256 ;
            bs_write_se(b, delta_scale);
        }

        lastScale = scalingList[ j ];
    }
}


//7.3.2.11 RBSP trailing bits syntax
static void write_rbsp_trailing_bits(bs_t* b)
{
    int rbsp_stop_one_bit = 1;
    int rbsp_alignment_zero_bit = 0;

    bs_write_f(b,1, rbsp_stop_one_bit); // equal to 1
    while( !bs_byte_aligned(b) )
    {
        bs_write_f(b,1, rbsp_alignment_zero_bit); // equal to 0
    }
}

//7.3.2.1 Sequence parameter set RBSP syntax
//return sps的长度，-1表示失败。sps_buf为sps数据，buf_size为数据大小。sps_buf为输出
int enc_seq_parameter_set_rbsp(sps_t* sps, uint8_t* sps_buf, int buf_size)
{
    int i;

    bs_t* b = NULL;
    int   buf_len = 0;
    uint8_t tmp_sps_buf[MAX_SPS_BUF_LEN] = "";

    if ((NULL == sps) || (NULL == sps_buf))
    {
        input_error
        return -1;
    }
    b = bs_new(tmp_sps_buf, sizeof(tmp_sps_buf));
    if (NULL == b)
    {
        elog("Alloc bs_new is fail!");
        return -1;
    }

    bs_write_u8(b, sps->profile_idc);
    bs_write_u1(b, sps->constraint_set0_flag);
    bs_write_u1(b, sps->constraint_set1_flag);
    bs_write_u1(b, sps->constraint_set2_flag);
    bs_write_u1(b, sps->constraint_set3_flag);
    bs_write_u1(b, sps->constraint_set4_flag);
    bs_write_u1(b, sps->constraint_set5_flag);
    bs_write_u(b,2, 0);  /* reserved_zero_2bits */
    bs_write_u8(b, sps->level_idc);
    bs_write_ue(b, sps->seq_parameter_set_id);
    if( sps->profile_idc == 100 || sps->profile_idc == 110 ||
        sps->profile_idc == 122 || sps->profile_idc == 144 )
    {
        bs_write_ue(b, sps->chroma_format_idc);
        if( sps->chroma_format_idc == 3 )
        {
            bs_write_u1(b, sps->residual_colour_transform_flag);
        }
        bs_write_ue(b, sps->bit_depth_luma_minus8);
        bs_write_ue(b, sps->bit_depth_chroma_minus8);
        bs_write_u1(b, sps->qpprime_y_zero_transform_bypass_flag);
        bs_write_u1(b, sps->seq_scaling_matrix_present_flag);
        if( sps->seq_scaling_matrix_present_flag )
        {
            for( i = 0; i < 8; i++ )
            {
                bs_write_u1(b, sps->seq_scaling_list_present_flag[ i ]);
                if( sps->seq_scaling_list_present_flag[ i ] )
                {
                    if( i < 6 )
                    {
                        write_scaling_list( b, sps->ScalingList4x4[ i ], 16,
                                      sps->UseDefaultScalingMatrix4x4Flag[ i ]);
                    }
                    else
                    {
                        write_scaling_list( b, sps->ScalingList8x8[ i - 6 ], 64,
                                      sps->UseDefaultScalingMatrix8x8Flag[ i - 6 ] );
                    }
                }
            }
        }
    }
    bs_write_ue(b, sps->log2_max_frame_num_minus4);
    bs_write_ue(b, sps->pic_order_cnt_type);
    if( sps->pic_order_cnt_type == 0 )
    {
        bs_write_ue(b, sps->log2_max_pic_order_cnt_lsb_minus4);
    }
    else if( sps->pic_order_cnt_type == 1 )
    {
        bs_write_u1(b, sps->delta_pic_order_always_zero_flag);
        bs_write_se(b, sps->offset_for_non_ref_pic);
        bs_write_se(b, sps->offset_for_top_to_bottom_field);
        bs_write_ue(b, sps->num_ref_frames_in_pic_order_cnt_cycle);
        for( i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++ )
        {
            bs_write_se(b, sps->offset_for_ref_frame[ i ]);
        }
    }
    bs_write_ue(b, sps->num_ref_frames);
    bs_write_u1(b, sps->gaps_in_frame_num_value_allowed_flag);
    bs_write_ue(b, sps->pic_width_in_mbs_minus1);
    bs_write_ue(b, sps->pic_height_in_map_units_minus1);
    bs_write_u1(b, sps->frame_mbs_only_flag);
    if( !sps->frame_mbs_only_flag )
    {
        bs_write_u1(b, sps->mb_adaptive_frame_field_flag);
    }
    bs_write_u1(b, sps->direct_8x8_inference_flag);
    bs_write_u1(b, sps->frame_cropping_flag);
    if( sps->frame_cropping_flag )
    {
        bs_write_ue(b, sps->frame_crop_left_offset);
        bs_write_ue(b, sps->frame_crop_right_offset);
        bs_write_ue(b, sps->frame_crop_top_offset);
        bs_write_ue(b, sps->frame_crop_bottom_offset);
    }
    bs_write_u1(b, sps->vui_parameters_present_flag);
    if( sps->vui_parameters_present_flag )
    {
        write_vui_parameters(sps, b);
    }
    write_rbsp_trailing_bits( b);

    buf_len = bs_pos(b);
    bs_free(b);

    buf_len = start_code_emulation_prevention_add(tmp_sps_buf, buf_len, sps_buf, buf_size);

    return buf_len;
}

//7.3.2.2 Picture parameter set RBSP syntax
int enc_pic_parameter_set_rbsp(pps_t* pps, uint8_t* pps_buf, int buf_size)
{
    int i;
    int i_group;
    uint8_t tmp_pps_buf[MAX_PPS_BUF_LEN] = "";

    bs_t* b = NULL;
    int buf_len = 0;

    if ((NULL == pps) || (NULL == pps_buf))
    {
        input_error
        return -1;
    }

    b = bs_new(tmp_pps_buf, sizeof(tmp_pps_buf));
    if (NULL == b)
    {
        elog("Alloc bs_new is fail!");
        return -1;
    }

    bs_write_ue(b, pps->pic_parameter_set_id);
    bs_write_ue(b, pps->seq_parameter_set_id);
    bs_write_u1(b, pps->entropy_coding_mode_flag);
    bs_write_u1(b, pps->pic_order_present_flag);
    bs_write_ue(b, pps->num_slice_groups_minus1);

    if( pps->num_slice_groups_minus1 > 0 )
    {
        bs_write_ue(b, pps->slice_group_map_type);
        if( pps->slice_group_map_type == 0 )
        {
            for( i_group = 0; i_group <= pps->num_slice_groups_minus1; i_group++ )
            {
                bs_write_ue(b, pps->run_length_minus1[ i_group ]);
            }
        }
        else if( pps->slice_group_map_type == 2 )
        {
            for( i_group = 0; i_group < pps->num_slice_groups_minus1; i_group++ )
            {
                bs_write_ue(b, pps->top_left[ i_group ]);
                bs_write_ue(b, pps->bottom_right[ i_group ]);
            }
        }
        else if( pps->slice_group_map_type == 3 ||
                 pps->slice_group_map_type == 4 ||
                 pps->slice_group_map_type == 5 )
        {
            bs_write_u1(b, pps->slice_group_change_direction_flag);
            bs_write_ue(b, pps->slice_group_change_rate_minus1);
        }
        else if( pps->slice_group_map_type == 6 )
        {
            bs_write_ue(b, pps->pic_size_in_map_units_minus1);
            for( i = 0; i <= pps->pic_size_in_map_units_minus1; i++ )
            {
                bs_write_u(b, intlog2( pps->num_slice_groups_minus1 + 1 ), pps->slice_group_id[ i ] ); // was u(v)
            }
        }
    }
    bs_write_ue(b, pps->num_ref_idx_l0_active_minus1);
    bs_write_ue(b, pps->num_ref_idx_l1_active_minus1);
    bs_write_u1(b, pps->weighted_pred_flag);
    bs_write_u(b,2, pps->weighted_bipred_idc);
    bs_write_se(b, pps->pic_init_qp_minus26);
    bs_write_se(b, pps->pic_init_qs_minus26);
    bs_write_se(b, pps->chroma_qp_index_offset);
    bs_write_u1(b, pps->deblocking_filter_control_present_flag);
    bs_write_u1(b, pps->constrained_intra_pred_flag);
    bs_write_u1(b, pps->redundant_pic_cnt_present_flag);

    if ( pps->_more_rbsp_data_present )
    {
        bs_write_u1(b, pps->transform_8x8_mode_flag);
        bs_write_u1(b, pps->pic_scaling_matrix_present_flag);
        if( pps->pic_scaling_matrix_present_flag )
        {
            for( i = 0; i < 6 + 2* pps->transform_8x8_mode_flag; i++ )
            {
                bs_write_u1(b, pps->pic_scaling_list_present_flag[ i ]);
                if( pps->pic_scaling_list_present_flag[ i ] )
                {
                    if( i < 6 )
                    {
                        write_scaling_list( b, pps->ScalingList4x4[ i ], 16,
                                      pps->UseDefaultScalingMatrix4x4Flag[ i ] );
                    }
                    else
                    {
                        write_scaling_list( b, pps->ScalingList8x8[ i - 6 ], 64,
                                      pps->UseDefaultScalingMatrix8x8Flag[ i - 6 ] );
                    }
                }
            }
        }
        bs_write_se(b, pps->second_chroma_qp_index_offset);
    }

    write_rbsp_trailing_bits(b);

    buf_len = bs_pos(b);
    bs_free(b);

    buf_len = start_code_emulation_prevention_add(tmp_pps_buf, buf_len, pps_buf, buf_size);

    return buf_len;
}

void debug_sps(sps_t* sps)
{
    printf("======= SPS =======\n");
    printf(" profile_idc : %d \n", sps->profile_idc );
    printf(" constraint_set0_flag : %d \n", sps->constraint_set0_flag );
    printf(" constraint_set1_flag : %d \n", sps->constraint_set1_flag );
    printf(" constraint_set2_flag : %d \n", sps->constraint_set2_flag );
    printf(" constraint_set3_flag : %d \n", sps->constraint_set3_flag );
    printf(" constraint_set4_flag : %d \n", sps->constraint_set4_flag );
    printf(" constraint_set5_flag : %d \n", sps->constraint_set5_flag );
    printf(" reserved_zero_2bits : %d \n", sps->reserved_zero_2bits );
    printf(" level_idc : %d \n", sps->level_idc );
    printf(" seq_parameter_set_id : %d \n", sps->seq_parameter_set_id );
    printf(" chroma_format_idc : %d \n", sps->chroma_format_idc );
    printf(" residual_colour_transform_flag : %d \n", sps->residual_colour_transform_flag );
    printf(" bit_depth_luma_minus8 : %d \n", sps->bit_depth_luma_minus8 );
    printf(" bit_depth_chroma_minus8 : %d \n", sps->bit_depth_chroma_minus8 );
    printf(" qpprime_y_zero_transform_bypass_flag : %d \n", sps->qpprime_y_zero_transform_bypass_flag );
    printf(" seq_scaling_matrix_present_flag : %d \n", sps->seq_scaling_matrix_present_flag );
    //  int seq_scaling_list_present_flag[8];
    //  void* ScalingList4x4[6];
    //  int UseDefaultScalingMatrix4x4Flag[6];
    //  void* ScalingList8x8[2];
    //  int UseDefaultScalingMatrix8x8Flag[2];
    printf(" log2_max_frame_num_minus4 : %d \n", sps->log2_max_frame_num_minus4 );
    printf(" pic_order_cnt_type : %d \n", sps->pic_order_cnt_type );
      printf("   log2_max_pic_order_cnt_lsb_minus4 : %d \n", sps->log2_max_pic_order_cnt_lsb_minus4 );
      printf("   delta_pic_order_always_zero_flag : %d \n", sps->delta_pic_order_always_zero_flag );
      printf("   offset_for_non_ref_pic : %d \n", sps->offset_for_non_ref_pic );
      printf("   offset_for_top_to_bottom_field : %d \n", sps->offset_for_top_to_bottom_field );
      printf("   num_ref_frames_in_pic_order_cnt_cycle : %d \n", sps->num_ref_frames_in_pic_order_cnt_cycle );
    //  int offset_for_ref_frame[256];
    printf(" num_ref_frames : %d \n", sps->num_ref_frames );
    printf(" gaps_in_frame_num_value_allowed_flag : %d \n", sps->gaps_in_frame_num_value_allowed_flag );
    printf(" pic_width_in_mbs_minus1 : %d \n", sps->pic_width_in_mbs_minus1 );
    printf(" pic_height_in_map_units_minus1 : %d \n", sps->pic_height_in_map_units_minus1 );
    printf(" frame_mbs_only_flag : %d \n", sps->frame_mbs_only_flag );
    printf(" mb_adaptive_frame_field_flag : %d \n", sps->mb_adaptive_frame_field_flag );
    printf(" direct_8x8_inference_flag : %d \n", sps->direct_8x8_inference_flag );
    printf(" frame_cropping_flag : %d \n", sps->frame_cropping_flag );
      printf("   frame_crop_left_offset : %d \n", sps->frame_crop_left_offset );
      printf("   frame_crop_right_offset : %d \n", sps->frame_crop_right_offset );
      printf("   frame_crop_top_offset : %d \n", sps->frame_crop_top_offset );
      printf("   frame_crop_bottom_offset : %d \n", sps->frame_crop_bottom_offset );
    printf(" vui_parameters_present_flag : %d \n", sps->vui_parameters_present_flag );

    printf("=== VUI ===\n");
    printf(" aspect_ratio_info_present_flag : %d \n", sps->vui.aspect_ratio_info_present_flag );
      printf("   aspect_ratio_idc : %d \n", sps->vui.aspect_ratio_idc );
        printf("     sar_width : %d \n", sps->vui.sar_width );
        printf("     sar_height : %d \n", sps->vui.sar_height );
    printf(" overscan_info_present_flag : %d \n", sps->vui.overscan_info_present_flag );
      printf("   overscan_appropriate_flag : %d \n", sps->vui.overscan_appropriate_flag );
    printf(" video_signal_type_present_flag : %d \n", sps->vui.video_signal_type_present_flag );
      printf("   video_format : %d \n", sps->vui.video_format );
      printf("   video_full_range_flag : %d \n", sps->vui.video_full_range_flag );
      printf("   colour_description_present_flag : %d \n", sps->vui.colour_description_present_flag );
        printf("     colour_primaries : %d \n", sps->vui.colour_primaries );
        printf("   transfer_characteristics : %d \n", sps->vui.transfer_characteristics );
        printf("   matrix_coefficients : %d \n", sps->vui.matrix_coefficients );
    printf(" chroma_loc_info_present_flag : %d \n", sps->vui.chroma_loc_info_present_flag );
      printf("   chroma_sample_loc_type_top_field : %d \n", sps->vui.chroma_sample_loc_type_top_field );
      printf("   chroma_sample_loc_type_bottom_field : %d \n", sps->vui.chroma_sample_loc_type_bottom_field );
    printf(" timing_info_present_flag : %d \n", sps->vui.timing_info_present_flag );
      printf("   num_units_in_tick : %d \n", sps->vui.num_units_in_tick );
      printf("   time_scale : %d \n", sps->vui.time_scale );
      printf("   fixed_frame_rate_flag : %d \n", sps->vui.fixed_frame_rate_flag );
    printf(" nal_hrd_parameters_present_flag : %d \n", sps->vui.nal_hrd_parameters_present_flag );
    printf(" vcl_hrd_parameters_present_flag : %d \n", sps->vui.vcl_hrd_parameters_present_flag );
      printf("   low_delay_hrd_flag : %d \n", sps->vui.low_delay_hrd_flag );
    printf(" pic_struct_present_flag : %d \n", sps->vui.pic_struct_present_flag );
    printf(" bitstream_restriction_flag : %d \n", sps->vui.bitstream_restriction_flag );
      printf("   motion_vectors_over_pic_boundaries_flag : %d \n", sps->vui.motion_vectors_over_pic_boundaries_flag );
      printf("   max_bytes_per_pic_denom : %d \n", sps->vui.max_bytes_per_pic_denom );
      printf("   max_bits_per_mb_denom : %d \n", sps->vui.max_bits_per_mb_denom );
      printf("   log2_max_mv_length_horizontal : %d \n", sps->vui.log2_max_mv_length_horizontal );
      printf("   log2_max_mv_length_vertical : %d \n", sps->vui.log2_max_mv_length_vertical );
      printf("   num_reorder_frames : %d \n", sps->vui.num_reorder_frames );
      printf("   max_dec_frame_buffering : %d \n", sps->vui.max_dec_frame_buffering );

    printf("=== HRD ===\n");
    printf(" cpb_cnt_minus1 : %d \n", sps->hrd.cpb_cnt_minus1 );
    printf(" bit_rate_scale : %d \n", sps->hrd.bit_rate_scale );
    printf(" cpb_size_scale : %d \n", sps->hrd.cpb_size_scale );
    int SchedSelIdx;
    for( SchedSelIdx = 0; SchedSelIdx <= sps->hrd.cpb_cnt_minus1; SchedSelIdx++ )
    {
        printf("   bit_rate_value_minus1[%d] : %d \n", SchedSelIdx, sps->hrd.bit_rate_value_minus1[SchedSelIdx] ); // up to cpb_cnt_minus1, which is <= 31
        printf("   cpb_size_value_minus1[%d] : %d \n", SchedSelIdx, sps->hrd.cpb_size_value_minus1[SchedSelIdx] );
        printf("   cbr_flag[%d] : %d \n", SchedSelIdx, sps->hrd.cbr_flag[SchedSelIdx] );
    }
    printf(" initial_cpb_removal_delay_length_minus1 : %d \n", sps->hrd.initial_cpb_removal_delay_length_minus1 );
    printf(" cpb_removal_delay_length_minus1 : %d \n", sps->hrd.cpb_removal_delay_length_minus1 );
    printf(" dpb_output_delay_length_minus1 : %d \n", sps->hrd.dpb_output_delay_length_minus1 );
    printf(" time_offset_length : %d \n", sps->hrd.time_offset_length );
}


void debug_pps(pps_t* pps)
{
    printf("======= PPS =======\n");
    printf(" pic_parameter_set_id : %d \n", pps->pic_parameter_set_id );
    printf(" seq_parameter_set_id : %d \n", pps->seq_parameter_set_id );
    printf(" entropy_coding_mode_flag : %d \n", pps->entropy_coding_mode_flag );
    printf(" pic_order_present_flag : %d \n", pps->pic_order_present_flag );
    printf(" num_slice_groups_minus1 : %d \n", pps->num_slice_groups_minus1 );
    printf(" slice_group_map_type : %d \n", pps->slice_group_map_type );
    //  int run_length_minus1[8]; // up to num_slice_groups_minus1, which is <= 7 in Baseline and Extended, 0 otheriwse
    //  int top_left[8];
    //  int bottom_right[8];
    //  int slice_group_change_direction_flag;
    //  int slice_group_change_rate_minus1;
    //  int pic_size_in_map_units_minus1;
    //  int slice_group_id[256]; // FIXME what size?
    printf(" num_ref_idx_l0_active_minus1 : %d \n", pps->num_ref_idx_l0_active_minus1 );
    printf(" num_ref_idx_l1_active_minus1 : %d \n", pps->num_ref_idx_l1_active_minus1 );
    printf(" weighted_pred_flag : %d \n", pps->weighted_pred_flag );
    printf(" weighted_bipred_idc : %d \n", pps->weighted_bipred_idc );
    printf(" pic_init_qp_minus26 : %d \n", pps->pic_init_qp_minus26 );
    printf(" pic_init_qs_minus26 : %d \n", pps->pic_init_qs_minus26 );
    printf(" chroma_qp_index_offset : %d \n", pps->chroma_qp_index_offset );
    printf(" deblocking_filter_control_present_flag : %d \n", pps->deblocking_filter_control_present_flag );
    printf(" constrained_intra_pred_flag : %d \n", pps->constrained_intra_pred_flag );
    printf(" redundant_pic_cnt_present_flag : %d \n", pps->redundant_pic_cnt_present_flag );
    printf(" transform_8x8_mode_flag : %d \n", pps->transform_8x8_mode_flag );
    printf(" pic_scaling_matrix_present_flag : %d \n", pps->pic_scaling_matrix_present_flag );
    //  int pic_scaling_list_present_flag[8];
    //  void* ScalingList4x4[6];
    //  int UseDefaultScalingMatrix4x4Flag[6];
    //  void* ScalingList8x8[2];
    //  int UseDefaultScalingMatrix8x8Flag[2];
    printf(" second_chroma_qp_index_offset : %d \n", pps->second_chroma_qp_index_offset );
}























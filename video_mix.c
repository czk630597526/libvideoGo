/*
 * video_mix.c
 *  目前支持1分屏，4分屏，6分屏和8分屏
 *  每个屏的序号如下：
1分屏：                                                                                                               4分屏：
-----------------------------------------        -----------------------------------------
|                                       |        |                  |                    |
|                                       |        |                  |                    |
|                                       |        |                  |                    |
|                                       |        |        1         |         2          |
|                                       |        |                  |                    |
|                                       |        |                  |                    |
|                  1                    |        -----------------------------------------
|                                       |        |                  |                    |
|                                       |        |                  |                    |
|                                       |        |                  |                    |
|                                       |        |        3         |         4          |
|                                       |        |                  |                    |
-----------------------------------------        |                  |                    |
                                                 -----------------------------------------
6分屏：                                                                                                               8分屏：
-----------------------------------------        -----------------------------------------
|                           |            |       |                            |          |
|                           |            |       |                            |     2    |
|                           |      2     |       |                            |          |
|                           |            |       |                            |-----------
           1                -------------        |                            |          |
|                           |            |       |             1              |     3    |
|                           |      3     |       |                            |          |
|                           |            |       |                            |-----------
|                           |            |       |                            |          |
-----------------------------------------        |                            |     4    |
|             |             |            |       |                            |          |
|      4      |       5     |      6     |       -----------------------------------------
|             |             |            |       |        |         |         |          |
|             |             |            |       |    5   |    6    |    7    |     8    |
-----------------------------------------        |        |         |         |          |
                                                 -----------------------------------------
 *  Created on: 2014-5-15
 *      Author: changgaowei
 */


#include "libswscale/swscale.h"
#include "libvideo.h"
#include "video_mix.h"


//将一个长度按照分屏个数tok_count分成多个长度，保存在lens中，lens的个数时len_count
//必须保证输出到lens中的值是16的整数倍。这是h264的要求。
//如果不能够整除，则将剩余的值放到其中部分中。
//示例：len = 1280，1280/16=80，80/3=26，余2，则输出：27*16，27*16，26*16
//:注意：可能会存在不是16倍数的情况，比如，1080，现在选择为8的倍数。
//成功返回0，失败返回-1
static int32_t get_len_list(int32_t len, int32_t tok_count, int32_t *lens, int32_t len_count)
{
    int32_t per_value = 0;  //商
    int32_t residual = 0;   //余数
    int32_t len_per8 = 0;  //len除以8所得到的值：可能会存在不是16倍数的情况，比如，1080，现在选择为8的倍数。
    int32_t i = 0;

    if ((tok_count) > len_count || (tok_count <=0) || (NULL == lens))
    {
        elog("get_len_list:input is error!");
        return -1 ;
    }

    len_per8 = len / 8;

    per_value = len_per8 / tok_count;
    if (per_value <= 0)
    {
        elog("video_mix:The len is too small[%d],can not get sub[%d] screen len.", len, tok_count);
        return -1;
    }
    residual = len_per8 % tok_count;

    for(i = 0; i < tok_count; i++)
    {
        if (i < residual)
        {
            lens[i] = (per_value + 1) * 8;
        }
        else
        {
            lens[i] = per_value * 8;
        }
    }
    return 0;
}

static int32_t init_full_type(int32_t out_width, int32_t out_height, VIDEO_SUB_INFO *sub_screens, int32_t sub_len)
{
    if (sub_len < 1)
    {
        elog("init_full_type:input is error!");
        return -1;
    }

    sub_screens[0].height = out_height;
    sub_screens[0].width = out_width;
    sub_screens[0].left_width = 0;
    sub_screens[0].above_height = 0;
    return 0;
}

static int32_t init_two_type(int32_t out_width, int32_t out_height, VIDEO_SUB_INFO *sub_screens, int32_t sub_len)
{
    int32_t widths[MAX_MIX_SCREEN_TOKEN_COUNT] = {0};
    int32_t heights[MAX_MIX_SCREEN_TOKEN_COUNT] = {0};
    int32_t screen_count = 2;
    int32_t tok_count = 4;  //2分屏的分割为4块。lens中len的个数。分四个，将一个屏变为16个，左边的是中间的四个合并

    if (sub_len < screen_count)
    {
        elog("init_full_type:input is error!");
        return -1;
    }

    memset(widths, 0, sizeof(widths));
    if (0 != get_len_list(out_width, tok_count, widths, sizeof(widths) / sizeof(widths[0])))
    {
        return -1;
    }

    memset(heights, 0, sizeof(heights));
    if (0 != get_len_list(out_height, tok_count, heights, sizeof(heights) / sizeof(heights[0])))
    {
        return -1;
    }

    sub_screens[0].width = widths[0] + widths[1];
    sub_screens[0].height = heights[1] + heights[2];
    sub_screens[0].left_width = 0;
    sub_screens[0].above_height = heights[0];

    sub_screens[1].width = widths[2] + widths[3];
    sub_screens[1].height = heights[1] + heights[2];
    sub_screens[1].left_width = widths[0] + widths[1];
    sub_screens[1].above_height = heights[0];

    return 0;
}

static int32_t init_three_type(int32_t out_width, int32_t out_height, VIDEO_SUB_INFO *sub_screens, int32_t sub_len)
{
    int32_t widths[MAX_MIX_SCREEN_TOKEN_COUNT] = {0};
    int32_t heights[MAX_MIX_SCREEN_TOKEN_COUNT] = {0};
    int32_t screen_count = 3;
    int32_t tok_count = 4;  //2分屏的分割为4块。lens中len的个数。分四个，将一个屏变为16个，左边的是中间的四个合并

    if (sub_len < screen_count)
    {
        elog("init_full_type:input is error!");
        return -1;
    }

    memset(widths, 0, sizeof(widths));
    if (0 != get_len_list(out_width, tok_count, widths, sizeof(widths) / sizeof(widths[0])))
    {
        return -1;
    }

    memset(heights, 0, sizeof(heights));
    if (0 != get_len_list(out_height, tok_count, heights, sizeof(heights) / sizeof(heights[0])))
    {
        return -1;
    }

    sub_screens[0].width = widths[1] + widths[2];
    sub_screens[0].height = heights[0] + heights[1];
    sub_screens[0].left_width = widths[0];
    sub_screens[0].above_height = 0;

    sub_screens[1].width = widths[0] + widths[1];
    sub_screens[1].height = heights[2] + heights[3];
    sub_screens[1].left_width = 0;
    sub_screens[1].above_height = heights[0] + heights[1];

    sub_screens[2].width = widths[2] + widths[3];
    sub_screens[2].height = heights[2] + heights[3];
    sub_screens[2].left_width = widths[0] + widths[1];
    sub_screens[2].above_height = heights[0] + heights[1];

    return 0;
}

static int32_t init_six_type(int32_t out_width, int32_t out_height, VIDEO_SUB_INFO *sub_screens, int32_t sub_len)
{
    int32_t widths[MAX_MIX_SCREEN_TOKEN_COUNT] = {0};
    int32_t heights[MAX_MIX_SCREEN_TOKEN_COUNT] = {0};
    int32_t screen_count = 6;
    int32_t tok_count = 3;   //六分屏的分割为三块。

    if (sub_len < screen_count)
    {
        elog("init_full_type:input is error!");
        return -1;
    }

    memset(widths, 0, sizeof(widths));
    if (0 != get_len_list(out_width, tok_count, widths, sizeof(widths) / sizeof(widths[0])))
    {
        return -1;
    }

    memset(heights, 0, sizeof(heights));
    if (0 != get_len_list(out_height, tok_count, heights, sizeof(heights) / sizeof(heights[0])))
    {
        return -1;
    }

    //这里选用了新的算法，灵活性比上面更高。解耦了各个屏之间的耦合。
    sub_screens[0].width = widths[0] + widths[1];
    sub_screens[0].height = heights[0] + heights[1];
    sub_screens[0].left_width = 0;
    sub_screens[0].above_height = 0;

    sub_screens[1].width = widths[2];
    sub_screens[1].height = heights[0];
    sub_screens[1].left_width = widths[0] + widths[1];
    sub_screens[1].above_height = 0;

    sub_screens[2].width = widths[2];
    sub_screens[2].height = heights[1];
    sub_screens[2].left_width = widths[0] + widths[1];
    sub_screens[2].above_height = heights[0];

    sub_screens[3].width = widths[0];
    sub_screens[3].height = heights[2];
    sub_screens[3].left_width = 0;
    sub_screens[3].above_height = heights[0] + heights[1];

    sub_screens[4].width = widths[1];
    sub_screens[4].height = heights[2];
    sub_screens[4].left_width = widths[0];
    sub_screens[4].above_height = heights[0] + heights[1];

    sub_screens[5].width = widths[2];
    sub_screens[5].height = heights[2];
    sub_screens[5].left_width = widths[0] + widths[1];
    sub_screens[5].above_height = heights[0] + heights[1];

    return 0;
}

static int32_t init_eight_type(int32_t out_width, int32_t out_height, VIDEO_SUB_INFO *sub_screens, int32_t sub_len)
{
    int32_t widths[MAX_MIX_SCREEN_TOKEN_COUNT] = {0};
    int32_t heights[MAX_MIX_SCREEN_TOKEN_COUNT] = {0};
    int32_t screen_count = 8;
    int32_t tok_count = 4;  ////16分屏的分割为四块，再组合。

    if (sub_len < screen_count)
    {
        elog("init_full_type:input is error!");
        return -1;
    }

    memset(widths, 0, sizeof(widths));
    if (0 != get_len_list(out_width, tok_count, widths, sizeof(widths) / sizeof(widths[0])))
    {
        return -1;
    }

    memset(heights, 0, sizeof(heights));
    if (0 != get_len_list(out_height, tok_count, heights, sizeof(heights) / sizeof(heights[0])))
    {
        return -1;
    }

    //这里选用了新的算法，灵活性比上面更高。解耦了各个屏之间的耦合，上面几个有时间在改。
    sub_screens[0].width = widths[0] + widths[1] + widths[2];
    sub_screens[0].height = heights[0] + heights[1] + heights[2];
    sub_screens[0].left_width = 0;
    sub_screens[0].above_height = 0;

    sub_screens[1].width = widths[3];
    sub_screens[1].height = heights[0];
    sub_screens[1].left_width = widths[0] + widths[1] + widths[2];
    sub_screens[1].above_height = 0;

    sub_screens[2].width = widths[3];
    sub_screens[2].height = heights[1];
    sub_screens[2].left_width = widths[0] + widths[1] + widths[2];
    sub_screens[2].above_height = heights[0];;

    sub_screens[3].width = widths[3];
    sub_screens[3].height = heights[2];
    sub_screens[3].left_width = widths[0] + widths[1] + widths[2];
    sub_screens[3].above_height = heights[0] + heights[1];

    sub_screens[4].width = widths[0];
    sub_screens[4].height = heights[3];
    sub_screens[4].left_width = 0;
    sub_screens[4].above_height = heights[0] + heights[1] + heights[2];

    sub_screens[5].width = widths[1];
    sub_screens[5].height = heights[3];
    sub_screens[5].left_width = widths[0];
    sub_screens[5].above_height = heights[0] + heights[1] + heights[2];

    sub_screens[6].width = widths[2];
    sub_screens[6].height = heights[3];
    sub_screens[6].left_width = widths[0] + widths[1];
    sub_screens[6].above_height = heights[0] + heights[1] + heights[2];

    sub_screens[7].width = widths[3];
    sub_screens[7].height = heights[3];
    sub_screens[7].left_width = widths[0] + widths[1] + widths[2];
    sub_screens[7].above_height = heights[0] + heights[1] + heights[2];

    return 0;
}

static int32_t init_thirteen_type(int32_t out_width, int32_t out_height, VIDEO_SUB_INFO *sub_screens, int32_t sub_len)
{
    int32_t widths[MAX_MIX_SCREEN_TOKEN_COUNT] = {0};
    int32_t heights[MAX_MIX_SCREEN_TOKEN_COUNT] = {0};
    int32_t screen_count = 13;
    int32_t tok_count = 4;  ////16分屏的分割为四块，再组合。

    if (sub_len < screen_count)
    {
        elog("init_full_type:input is error!");
        return -1;
    }

    memset(widths, 0, sizeof(widths));
    if (0 != get_len_list(out_width, tok_count, widths, sizeof(widths) / sizeof(widths[0])))
    {
        return -1;
    }

    memset(heights, 0, sizeof(heights));
    if (0 != get_len_list(out_height, tok_count, heights, sizeof(heights) / sizeof(heights[0])))
    {
        return -1;
    }

    //这里选用了新的算法，灵活性比上面更高。解耦了各个屏之间的耦合，上面几个有时间在改。
    sub_screens[0].width = widths[0];
    sub_screens[0].height = heights[0];
    sub_screens[0].left_width = 0;
    sub_screens[0].above_height = 0;

    sub_screens[1].width = widths[1];
    sub_screens[1].height = heights[0];
    sub_screens[1].left_width = widths[0];
    sub_screens[1].above_height = 0;

    sub_screens[2].width = widths[2];
    sub_screens[2].height = heights[0];
    sub_screens[2].left_width = widths[0] + widths[1];
    sub_screens[2].above_height = 0;

    sub_screens[3].width = widths[3];
    sub_screens[3].height = heights[0];
    sub_screens[3].left_width = widths[0] + widths[1] + widths[2];
    sub_screens[3].above_height = 0;

    sub_screens[4].width = widths[0];
    sub_screens[4].height = heights[1];
    sub_screens[4].left_width = 0;
    sub_screens[4].above_height = heights[0];

    sub_screens[5].width = widths[1] + widths[2];
    sub_screens[5].height = heights[1] + heights[2];
    sub_screens[5].left_width = widths[0];
    sub_screens[5].above_height = heights[0];

    sub_screens[6].width = widths[3];
    sub_screens[6].height = heights[1];
    sub_screens[6].left_width = widths[0] + widths[1] + widths[2];
    sub_screens[6].above_height = heights[0];

    sub_screens[7].width = widths[0];
    sub_screens[7].height = heights[2];
    sub_screens[7].left_width = 0;
    sub_screens[7].above_height = heights[0] + heights[1];

    sub_screens[8].width = widths[3];
    sub_screens[8].height = heights[2];
    sub_screens[8].left_width = widths[0] + widths[1] + widths[2];
    sub_screens[8].above_height = heights[0] + heights[1];

    sub_screens[9].width = widths[0];
    sub_screens[9].height = heights[3];
    sub_screens[9].left_width = 0;
    sub_screens[9].above_height = heights[0] + heights[1] + heights[2];

    sub_screens[10].width = widths[1];
    sub_screens[10].height = heights[3];
    sub_screens[10].left_width = widths[0];
    sub_screens[10].above_height = heights[0] + heights[1] + heights[2];

    sub_screens[11].width = widths[2];
    sub_screens[11].height = heights[3];
    sub_screens[11].left_width = widths[0] + widths[1] ;
    sub_screens[11].above_height = heights[0] + heights[1] + heights[2];

    sub_screens[12].width = widths[3];
    sub_screens[12].height = heights[3];
    sub_screens[12].left_width = widths[0] + widths[1] + widths[2];
    sub_screens[12].above_height = heights[0] + heights[1] + heights[2];

    return 0;
}

//初始化规则的分屏：包括：4，9，16。screen_count是分屏个数；tok_count：纵横分割个数，4分屏填2，9分屏填3，16分屏为4。
static int32_t init_regular_type(int32_t out_width, int32_t out_height, VIDEO_SUB_INFO *sub_screens, int32_t sub_len,
                                int32_t screen_count, int32_t tok_count)
{
    int32_t widths[MAX_MIX_SCREEN_TOKEN_COUNT] = {0};
    int32_t heights[MAX_MIX_SCREEN_TOKEN_COUNT] = {0};
    int32_t iw = 0;
    int32_t ih = 0;
    int32_t ileft_width_total = 0;
    int32_t iabove_heigh_total = 0;

    if ((sub_len < screen_count) || (tok_count > MAX_MIX_SCREEN_TOKEN_COUNT))
    {
        elog("init_full_type:input is error!");
        return -1;
    }

    memset(widths, 0, sizeof(widths));
    if (0 != get_len_list(out_width, tok_count, widths, sizeof(widths) / sizeof(widths[0])))
    {
        return -1;
    }

    memset(heights, 0, sizeof(heights));
    if (0 != get_len_list(out_height, tok_count, heights, sizeof(heights) / sizeof(heights[0])))
    {
        return -1;
    }

    for (ih = 0; ih < tok_count; ih++)
    {
        ileft_width_total = 0;  //每次都置为0
        for (iw = 0; iw < tok_count; iw++)
        {
            sub_screens[ih * tok_count + iw].width = widths[iw];
            sub_screens[ih * tok_count + iw].height = heights[ih];
            sub_screens[ih * tok_count + iw].left_width = ileft_width_total;
            sub_screens[ih * tok_count + iw].above_height = iabove_heigh_total;

            ileft_width_total += widths[iw];
        }
        iabove_heigh_total += heights[ih];
    }

    return 0;
}

int Reset_video_mix(VIDEO_MIX_TH *pvmth,int num){
    if(pvmth==NULL){
        return -1;
    }
    int out_width = pvmth->out_width;
    int out_height = pvmth->out_height;
    int iret = 0;
    switch(num)
    {
        case VIDEO_MIX_FULL:
            iret = init_full_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT);
            break;
        case VIDEO_MIX_TWO:
            iret = init_two_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT);
            break;
        case VIDEO_MIX_THREE:
            iret = init_three_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT);
            break;
        case VIDEO_MIX_FOUR:
            iret = init_regular_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT, 4, 2);
            break;
        case VIDEO_MIX_SIX:
            iret = init_six_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT);
            break;
        case VIDEO_MIX_EIGHT:
            iret = init_eight_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT);
            break;
        case VIDEO_MIX_NINE:
            iret = init_regular_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT, 9, 3);
            break;
        case VIDEO_MIX_THIRTEEN:
            iret = init_thirteen_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT);
            break;
        case VIDEO_MIX_SIXTEEN:
            iret = init_regular_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT, 16, 4);
            break;
        case VIDEO_MIX_TWENTY_FIVE:
            iret = init_regular_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT, 25, 5);
            break;
        default:
            iret = -1;
            elog("Unkonwn mix type:[%d]", num);
            break;
    }
    if (iret == -1){

    }else{
        pvmth->mix_type = num;
    }
    return iret;
}


VIDEO_MIX_TH *alloc_video_mix_th(VIDEO_MIX_TYPE mix_type, int32_t out_width, int32_t out_height, PICTURE_FORMAT pic_fmt)
{
    int32_t iret = 0;

    if (pic_fmt != PIC_FMT_YUV_I420)
    {
        elog("alloc_video_mix_th:PICTURE_FORMAT pic_fmt is not PIC_FMT_YUV_I420!");
        return NULL;
    }
    VIDEO_MIX_TH* pvmth = (VIDEO_MIX_TH *)malloc(sizeof(VIDEO_MIX_TH));
    if (NULL == pvmth)
    {
        elog("alloc_video_mix_th:malloc is fail!");
        return NULL;
    }

    memset(pvmth, 0, sizeof(VIDEO_MIX_TH));

    pvmth->mix_type = mix_type;
    pvmth->out_height = out_height;
    pvmth->out_width = out_width;
    pvmth->out_pic_fmt = pic_fmt;

    int i = 0;
    for (i = 0; i < MAX_MIX_SCREEN_COUNT; i++)
    {
        pvmth->sub_screens[i].sws_ctx_ffm = NULL;
    }

    switch(mix_type)
    {
        case VIDEO_MIX_FULL:
            iret = init_full_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT);
            break;
        case VIDEO_MIX_TWO:
            iret = init_two_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT);
            break;
        case VIDEO_MIX_THREE:
            iret = init_three_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT);
            break;
        case VIDEO_MIX_FOUR:
            iret = init_regular_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT, 4, 2);
            break;
        case VIDEO_MIX_SIX:
            iret = init_six_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT);
            break;
        case VIDEO_MIX_EIGHT:
            iret = init_eight_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT);
            break;
        case VIDEO_MIX_NINE:
            iret = init_regular_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT, 9, 3);
            break;
        case VIDEO_MIX_THIRTEEN:
            iret = init_thirteen_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT);
            break;
        case VIDEO_MIX_SIXTEEN:
            iret = init_regular_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT, 16, 4);
            break;
        case VIDEO_MIX_TWENTY_FIVE:
            iret = init_regular_type(out_width, out_height, pvmth->sub_screens, MAX_MIX_SCREEN_COUNT, 25, 5);
            break;
        default:
            iret = -1;
            elog("Unkonwn mix type:[%d]", mix_type);
            break;
    }

    if (iret != 0)
    {
        free_video_mix_th(pvmth);
        return NULL;
    }
    return pvmth;
}

void free_video_mix_th(VIDEO_MIX_TH *pvmth)
{
    if (pvmth)
    {
        int i = 0;
        for (i = 0; i < MAX_MIX_SCREEN_COUNT; i++)
        {
            if (NULL != pvmth->sub_screens[i].sws_ctx_ffm)
            {
                sws_freeContext(pvmth->sub_screens[i].sws_ctx_ffm);
            }
        }
        free(pvmth);
    }
}

static int32_t get_screen_count_by_mix_type(VIDEO_MIX_TYPE mix_type)
{
    switch(mix_type)
    {
        case VIDEO_MIX_FULL:
            return 1;
            break;
        case VIDEO_MIX_TWO:
            return 2;
            break;
        case VIDEO_MIX_THREE:
            return 3;
            break;
        case VIDEO_MIX_FOUR:
            return 4;
            break;
        case VIDEO_MIX_SIX:
            return 6;
            break;
        case VIDEO_MIX_EIGHT:
            return 8;
            break;
        case VIDEO_MIX_NINE:
            return 9;
            break;
        case VIDEO_MIX_THIRTEEN:
            return 13;
            break;
        case VIDEO_MIX_SIXTEEN:
            return 16;
            break;
        case VIDEO_MIX_TWENTY_FIVE:
            return 25;
            break;
        default:
            return  -1;
            elog("Unkonwn mix type:[%d]", mix_type);
            break;
    }
}

static int32_t video_process_sub_screen(VIDEO_SUB_INFO *psub_screen, FRAME_PIC_DATA *in_pic, FRAME_PIC_DATA *out_pic)
{
    if ((NULL == psub_screen) || (out_pic == NULL) || (in_pic == NULL))
    {
        elog("video_process_sub_screen:input is error!");
        return -1;
    }

    if ((psub_screen->sws_ctx_ffm != NULL) //输入的长度和高度发生改变。重新申请。
            && ((psub_screen->in_width != in_pic->width) || (psub_screen->in_height != in_pic->height)))
    {
        sws_freeContext(psub_screen->sws_ctx_ffm);
        psub_screen->sws_ctx_ffm = NULL;
        psub_screen->in_width = 0;
        psub_screen->in_height = 0;
    }

    if (psub_screen->sws_ctx_ffm == NULL)   //为NULL，则重新申请。
    {
        psub_screen->sws_ctx_ffm = sws_getContext(in_pic->width, in_pic->height, AV_PIX_FMT_YUV420P,
                                            psub_screen->width, psub_screen->height, AV_PIX_FMT_YUV420P,
                                            SWS_FAST_BILINEAR, NULL, NULL, NULL);
        if (psub_screen->sws_ctx_ffm == NULL)
        {
            elog("Alloc sws_getContext is error!");
            return -1;
        }
        psub_screen->in_width = in_pic->width;
        psub_screen->in_height = in_pic->height;
    }

    uint8_t *dst_data[LIBV_NUM_DATA_POINTERS];
    int32_t dst_linesize[LIBV_NUM_DATA_POINTERS];
    //获取y的其实位置。
    dst_data[0] = out_pic->data[0] + psub_screen->above_height * out_pic->linesize[0] + psub_screen->left_width;
    dst_linesize[0] = out_pic->linesize[0];

    //获取u和v的位置
    dst_data[1] = out_pic->data[1] + psub_screen->above_height / 2 * out_pic->linesize[1] + psub_screen->left_width / 2;
    dst_linesize[1] = out_pic->linesize[1];
    dst_data[2] = out_pic->data[2] + psub_screen->above_height / 2 * out_pic->linesize[2] + psub_screen->left_width / 2;
    dst_linesize[2] = out_pic->linesize[2];

    dst_data[3] = NULL;
    dst_linesize[3] = 0;

    sws_scale(psub_screen->sws_ctx_ffm, (const uint8_t * const*)in_pic->data,
                in_pic->linesize, 0, in_pic->height, dst_data, dst_linesize);
    return 0;
}

int32_t video_mix_pic(VIDEO_MIX_TH *pvmth, FRAME_PIC_DATA **in_pics, int32_t in_count, FRAME_PIC_DATA *out_pic)
{
    if ((NULL == pvmth) || (in_pics == NULL) || (out_pic == NULL))
    {
        elog("video_mix_pic:input is error!");
        return -1;
    }

    int32_t screen_count = get_screen_count_by_mix_type(pvmth->mix_type);
    if (screen_count <= 0)
    {
        elog("get screen count by mix type[%d] error.", pvmth->mix_type);
        return -1;
    }

    if (in_count < screen_count)
    {
        elog("video mix: request in picture count[%d], but input count[%d].", screen_count, in_count);
        return -1;
    }

    if ((out_pic->width != pvmth->out_width) || (out_pic->height != pvmth->out_height)
            || (out_pic->pic_format != pvmth->out_pic_fmt))
    {
        elog("out picter is not init VIDEO_MIX_TH value.");
        return -1;
    }

    int32_t i = 0;
    VIDEO_SUB_INFO *psub_screen = pvmth->sub_screens;
    for (i = 0; i < screen_count; i ++)
    {
        if (NULL != in_pics[i])
        {
            video_process_sub_screen(&psub_screen[i], in_pics[i], out_pic);
        }
    }

    return 0;
}


























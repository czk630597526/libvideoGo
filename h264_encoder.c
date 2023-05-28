/*
 * h264_encoder.c
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h264_encoder.h"
#include "openh264_encoder.h"

#define  MAX_ENCODER_COUNT   8

static H264_ENCODER_FACTORY *gp_encoder_factorys[MAX_ENCODER_COUNT] = {0};

int32_t h264_encoder_register(H264_ENCODER_FACTORY *penc_fact)
{
    int32_t i = 0;
    for (i = 0; i < MAX_ENCODER_COUNT; i++)
    {
        if (gp_encoder_factorys[i] == NULL)
        {
            gp_encoder_factorys[i] = penc_fact;
            return 0;
        }
    }
    return -1;
}

static H264_ENCODER_FACTORY *h264_encoder_find_by_codec_id(H264_ENCODER_ID id)
{
    int32_t i = 0;
    for (i = 0; i < MAX_ENCODER_COUNT; i++)
    {
        if ((gp_encoder_factorys[i] != NULL) && (gp_encoder_factorys[i]->id == id))
        {
            return gp_encoder_factorys[i];
        }
    }
    return NULL;
}

void h264_encoder_init()
{
    int32_t i = 0;
    for (i = 0; i < MAX_ENCODER_COUNT; i++)
    {
        gp_encoder_factorys[i] = NULL;
    }

    openh264_encoder_init();
}

H264_ENCODER_TH *alloc_encoder_by_id(H264_ENCODER_ID id, ENCODE_PARAMETER *pparam)
{
    if (NULL == pparam)
    {
        elog("alloc_encoder_by_id:input is error.");
        return NULL;
    }

    H264_ENCODER_FACTORY * pfact = h264_encoder_find_by_codec_id(id);
    if (NULL == pfact)
    {
        elog("Can not get encoder factory by decoder id[%d].", id);
        return NULL;
    }

    H264_ENCODER_TH * pencth = (H264_ENCODER_TH *)malloc(sizeof(struct __H264_ENCODER_TH));
    if (NULL == pencth)
    {
        elog("Alloc memery fail!");
        return NULL;
    }

    memset(pencth, 0, sizeof(struct __H264_ENCODER_TH));

    pencth->_id = id;
    if (0 != pfact->init(pencth, pparam))
    {
        elog("__H264_ENCODER_TH initial by factory is fail!");
        free(pencth);
        return NULL;
    }

    return pencth;
}

void free_encoder(H264_ENCODER_TH *pencth)
{
    H264_ENCODER_FACTORY * pfact = NULL;

    if (NULL == pencth)
    {
        elog("free_encoder:input is error!");
        return ;
    }

    pfact = h264_encoder_find_by_codec_id(pencth->_id);
    if (NULL == pfact)
    {
        elog("Can not get encoder factory by decoder id[%d]. Is posible memery leak!", pencth->_id);
        free(pencth);
        return ;
    }

    pfact->release(pencth);
    free(pencth);
}


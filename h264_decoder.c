/*
 * h264_decoder.c
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h264_decoder.h"
#include "openh264_decoder.h"
#include "ffm_decoder.h"

#define  MAX_DECODER_COUNT   8

static H264_DECODER_FACTORY *gp_decoder_factorys[MAX_DECODER_COUNT] = {0};

int32_t h264_decoder_register(H264_DECODER_FACTORY *pdec_fact)
{
    int32_t i = 0;
    for (i = 0; i < MAX_DECODER_COUNT; i++)
    {
        if (gp_decoder_factorys[i] == NULL)
        {
            gp_decoder_factorys[i] = pdec_fact;
            return 0;
        }
    }
    return -1;
}

static H264_DECODER_FACTORY *h264_decoder_find_by_codec_id(H264_DECODER_ID id)
{
    int32_t i = 0;
    for (i = 0; i < MAX_DECODER_COUNT; i++)
    {
        if ((gp_decoder_factorys[i] != NULL) && (gp_decoder_factorys[i]->id == id))
        {
            return gp_decoder_factorys[i];
        }
    }
    return NULL;
}

void h264_decoder_init()
{
    int32_t i = 0;
    for (i = 0; i < MAX_DECODER_COUNT; i++)
    {
        gp_decoder_factorys[i] = NULL;
    }

    ffm_decoder_init();

    openh264_decoder_init();
}

H264_DECODER_TH *alloc_decoder_by_id(H264_DECODER_ID id)
{
    H264_DECODER_FACTORY * pfact = h264_decoder_find_by_codec_id(id);
    if (NULL == pfact)
    {
        elog("Can not get decoder factory by decoder id[%d].", id);
        return NULL;
    }

    H264_DECODER_TH * pdecth = (H264_DECODER_TH *)malloc(sizeof(struct __H264_DECODER_TH));
    if (NULL == pdecth)
    {
        elog("Alloc memery fail!");
        return NULL;
    }

    memset(pdecth, 0, sizeof(struct __H264_DECODER_TH));

    pdecth->_id = id;
    if (0 != pfact->init(pdecth))
    {
        elog("H264_DECODER_TH initial by factory is fail!");
        free(pdecth);
        return NULL;
    }

    return pdecth;
}

void free_decoder(H264_DECODER_TH *pdecth)
{
    H264_DECODER_FACTORY * pfact = NULL;

    if (NULL == pdecth)
    {
        elog("free_decoder:input is error!");
        return ;
    }

    pfact = h264_decoder_find_by_codec_id(pdecth->_id);
    if (NULL == pfact)
    {
        elog("Can not get decoder factory by decoder id[%d]. Is posible memery leak!", pdecth->_id);
        free(pdecth);
        return ;
    }

    pfact->release(pdecth);
    free(pdecth);
}













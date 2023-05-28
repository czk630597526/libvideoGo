/*
 * openh264_decoder.h
 *
 */

#ifndef OPENH264_DECODER_H_
#define OPENH264_DECODER_H_


#include <stdint.h>
#include "libv_log.h"
#include "libvideo.h"

#ifdef __cplusplus
extern "C" {
#endif


void openh264_decoder_init();    //上层不用调用，会在libvideo_init中调用。

#ifdef __cplusplus
}
#endif


#endif /* OPENH264_DECODER_H_ */

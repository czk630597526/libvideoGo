/*
 * openh264_encoder.h
 *
 */

#ifndef OPENH264_ENCODER_H_
#define OPENH264_ENCODER_H_

#include <stdint.h>
#include "libv_log.h"
#include "libvideo.h"

#ifdef __cplusplus
extern "C" {
#endif

void openh264_encoder_init();    //上层不用调用，会在libvideo_init中调用。


#ifdef __cplusplus
}
#endif


#endif /* OPENH264_ENCODER_H_ */

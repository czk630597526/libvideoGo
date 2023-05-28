%module libvideo
%{
/* Includes the header in the wrapper code */
#include "libvideo.h"
#include "video_mix.h"
#include "h264_rtp.h"
#include "h264_decoder.h"
#include "h264_encoder.h"
#include "ffm_decoder.h"
%}
 
/* Parse the header file to generate wrappers */
%include "libvideo.h"
%include "video_mix.h"
%include "h264_rtp.h"
%include "h264_decoder.h"
%include "h264_encoder.h"
%include "ffm_decoder.h"


/*command: swig4 -go -intgosize 64 -cgo libvideo.i */


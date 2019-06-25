/**
 * H264 decoder for WebAssembly
 * Jiang Dunchun (jiangdunchun@outlook.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <emscripten/emscripten.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

// 0-3 width; 4-7 height; 8-11 ptr; 12-12+width*height*x data
typedef struct {
	uint32_t width;
	uint32_t height;
	uint8_t *data;
} ImageData;

AVCodec *_codec;
AVCodecContext *_codecCtx= NULL;
AVFrame *_frameYUV;
AVPacket _packet;
ImageData *_imageData;
AVHWAccel *_accel;

// use yuv as the image data, data length is width*height*3/2
void parse_yuv420_image(AVFrame *frame_yuv, ImageData *image_data){
	image_data->width = (uint32_t)frame_yuv->width;
	image_data->height = (uint32_t)frame_yuv->height;

	uint8_t *buffer = (uint8_t *)malloc(frame_yuv->width * frame_yuv->height * 3 / 2);
	int index = 0;
	for (int y = 0; y < frame_yuv->height; y++){
		memcpy(buffer+index, frame_yuv->data[0] + y * frame_yuv->linesize[0], frame_yuv->width);
		index += frame_yuv->width;
	}
	for (int y = 0; y < frame_yuv->height/2; y++){
		memcpy(buffer+index, frame_yuv->data[1] + y * frame_yuv->linesize[1], frame_yuv->width/2);
		index += frame_yuv->width/2;
	}
	for (int y = 0; y < frame_yuv->height/2; y++){
		memcpy(buffer+index, frame_yuv->data[2] + y * frame_yuv->linesize[2], frame_yuv->width/2);
		index += frame_yuv->width/2;
	}

	image_data->data = buffer;
}

// find the hardware acceleration of this device
AVHWAccel *ff_find_hwaccel(enum AVCodecID codec_id, enum AVPixelFormat pix_fmt)
{
    AVHWAccel *hwaccel=NULL;
    
    while((hwaccel = av_hwaccel_next(hwaccel))){
		printf("[H264 decoder] hardware accelarationpatch with:%s\n",  hwaccel->name);
        if ( hwaccel->id == codec_id && hwaccel->pix_fmt == pix_fmt)
            return hwaccel;
    }
    return NULL;
}

// init the decoder
EMSCRIPTEN_KEEPALIVE
int init_decoder(void){
	avcodec_register_all();

	_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!_codec) {
		printf("[H264 decoder] codec not found\n");
		return -11;
	}

	_codecCtx = avcodec_alloc_context3(_codec);
	if (!_codecCtx){
		printf("[H264 decoder] could not allocate video codec context\n");
		return -12;
	}

	if (avcodec_open2(_codecCtx, _codec, NULL) < 0) {
		printf("[H264 decoder] could not open codec\n");
		return -14;
	}

	_accel = ff_find_hwaccel(AV_CODEC_ID_H264, AV_PIX_FMT_YUV420P);
	if (_accel == NULL){
		printf("[H264 decoder] hardware accelaration not found\n");
	}
	else{
		printf("[H264 decoder] hardware accelaration:%s\n", _accel->name);
	}

	_frameYUV = av_frame_alloc();

	av_init_packet(&_packet);

	_imageData = (ImageData *)malloc(sizeof(ImageData));
	return 1;
}

// decode a frame
// @ need to free the p_ptr & _imageData->data after decoding a frame in the js file
EMSCRIPTEN_KEEPALIVE
ImageData *decode_one_frame(uint8_t* p_ptr, int p_length){
	_packet.data = p_ptr;
	_packet.size = p_length;

	int got_pic = -1;
	int ret = avcodec_decode_video2(_codecCtx, _frameYUV, &got_pic, &_packet);
	if (ret < 0){
		return NULL;
	}

	if (got_pic){
		parse_yuv420_image(_frameYUV, _imageData);
		return _imageData;
	}
	else{
		return NULL;
	}	
}

// shutdown the decoder
EMSCRIPTEN_KEEPALIVE
int shutdown_decoder(void){
	av_frame_free(&_frameYUV);

	avcodec_close(_codecCtx);
	av_free(_codecCtx);

	av_free_packet(&_packet);

	if (_imageData != NULL){
		if (_imageData->data != NULL){
			free(_imageData->data);
			_imageData->data = NULL;
		}
		free(_imageData);
	}

	printf("[H264 decoder] shutdown H264 decoder\n");
	return 1;
}

int main(int argc, char ** argv) {
	printf("[H264 decoder] for WebAssembly 201906251015\n");
	return 1;
}
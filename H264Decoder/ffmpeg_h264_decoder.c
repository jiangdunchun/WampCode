/**
 * H264 decoder for WebAssembly
 * Jiang Dunchun (jiangdunchun@outlook.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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
#ifdef DEBUG
int _frameCount = 0;
#endif

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

	_frameYUV = av_frame_alloc();

	av_init_packet(&_packet);

	_imageData = (ImageData *)malloc(sizeof(ImageData));
	return 1;
}

// decode a frame
// need to free the p_ptr & _imageData->data after decoding a frame in the js file
EMSCRIPTEN_KEEPALIVE
ImageData *decode_one_frame(uint8_t* p_ptr, int p_length){
#ifdef DEBUG
	printf("[H264 decoder] frame count:%i\n", _frameCount);
	_frameCount++;
#endif

#ifdef DEBUG
	// start time dianoge
	clock_t start, stop;
	start = clock();
#endif

	_packet.data = p_ptr;
	_packet.size = p_length;

	int send_erro = 0;
	int receive_erro = 0;
	do{
		send_erro = avcodec_send_packet(_codecCtx, &_packet);

		if (send_erro != 0){
			// printf("[H264 decoder] send_erro:%i\n", send_erro);
			// avcodec_flush_buffers(_codecCtx);
			break;
		}
		receive_erro = avcodec_receive_frame(_codecCtx, _frameYUV);
		if (receive_erro != 0 &&  receive_erro != 11){
			// printf("[H264 decoder] receive_erro:%i\n", receive_erro);
			// avcodec_flush_buffers(_codecCtx);
			break;
		}
	}while (receive_erro == 11);

	if (send_erro == 0&& receive_erro == 0){
		parse_yuv420_image(_frameYUV, _imageData);
	}

#ifdef DEBUG
	// end time dianoge
	stop = clock();
	double duration = ((double)(start - stop)) / 1000;
	printf("[H264 decoder] decode time:%lf\n", duration);
#endif
	return _imageData;
}

// // free the resource after drawing the frame
// EMSCRIPTEN_KEEPALIVE
// int free_source(void){
// 	av_free_packet(&_packet);

// 	free(_imageData->data);
// 	_imageData->data = NULL;
// 	return 1;
// }

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
	printf("[H264 decoder] for WebAssembly 201906231434\n");
	return 1;
}
H264 Decoder
==============

# Introduction
H264 decoder for WebAssembly

# Notice
1. clone the [ffmpeg](https://github.com/FFmpeg/FFmpeg)

2. copy the lib folder to ffmpeg folder

3. copy the ffmpeg_h264_decoder.c to ffmpeg folder

4. compile the c code to wasm
```
//cd to ffmpeg firstly
//compile c code

cd ffmpeg
emcc ffmpeg_h264_decoder.c -s WASM=1 -O3 -o H264Decoder.html -I./ ./lib/libavformat.bc ./lib/libavcodec.bc ./lib/libswscale.bc ./lib/libswresample.bc ./lib/libavutil.bc -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall"]' -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=16777216

//or compile c code in DEBUG

emcc ffmpeg_h264_decoder.c -DDEBUG -s WASM=1 -O3 -o H264Decoder.html -I./ ./lib/libavformat.bc ./lib/libavcodec.bc ./lib/libswscale.bc ./lib/libswresample.bc ./lib/libavutil.bc -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall"]' -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=16777216
```
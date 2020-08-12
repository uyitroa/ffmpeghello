//
// Created by yuitora . on 12/08/2020.
//

#ifndef FFMPEGHELLO_FRAMEWRITER_H
#define FFMPEGHELLO_FRAMEWRITER_H

extern "C" {
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

typedef struct OutputStream {
    AVStream *st;
    AVCodecContext *enc;

    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;

    AVFrame *frame;
    AVFrame *tmp_frame;

    float t, tincr, tincr2;

    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
} OutputStream;


class FrameWriter {

public:

    AVOutputFormat *fmt;
    AVFormatContext *oc;
    enum AVCodecID video_codec;
    enum AVPixelFormat pix_fmt;
    int width;
    int height;
    const char *filename;
    OutputStream video_st = {nullptr};
    bool closed;

    FrameWriter();
    FrameWriter(const char *filename, int width, int height, int fps);
    void open_video();
    int write_frame();
    AVFrame *alloc_picture();
    void close_video();
    uint8_t *getbuffer();
    int getbuffersize();
    ~FrameWriter();
};


#endif //FFMPEGHELLO_FRAMEWRITER_H

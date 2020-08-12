//
// Created by yuitora . on 12/08/2020.
//

#include "FrameWriter.h"

FrameWriter::FrameWriter(const char *filename, int width, int height, int fps) {

    this->filename = filename;
    this->width = width;
    this->height = height;
    this->closed = false;
    pix_fmt = AV_PIX_FMT_YUV420P;
    video_codec = AV_CODEC_ID_H264;

    AVCodecContext *c;
    int i;

    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    if (!oc) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    }
    if (!oc)
        exit(1);

    fmt = oc->oformat;

    video_st.st = avformat_new_stream(oc, NULL);
    if (!video_st.st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    video_st.st->id = oc->nb_streams-1;
    c = avcodec_alloc_context3(avcodec_find_encoder(video_codec));
    if (!c) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        exit(1);
    }

    video_st.enc = c;
    video_st.enc->codec_id = video_codec;

    video_st.enc->bit_rate = 400000;
    /* Resolution must be a multiple of two. */

    video_st.enc->width    = width;
    video_st.enc->height   = height;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
    video_st.st->time_base = (AVRational){ 1, fps };
    video_st.enc->time_base       = video_st.st->time_base;

    video_st.enc->gop_size      = 12; /* emit one intra frame every twelve frames at most */
    video_st.enc->pix_fmt       = pix_fmt;


    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

void FrameWriter::open_video() {
    int ret;
    AVCodecContext *c = video_st.enc;
    AVDictionary *opt = NULL;

//    av_dict_copy(&opt, opt_arg, 0);

    /* open the codec */
    ret = avcodec_open2(c, avcodec_find_encoder(video_codec), &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
        exit(1);
    }

    /* allocate and init a re-usable frame */
    video_st.frame = alloc_picture();
    if (!video_st.frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    /* If the output format is not YUV420P, then a temporary YUV420P
     * picture is needed too. It is then converted to the required
     * output format. */
    video_st.tmp_frame = NULL;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        video_st.tmp_frame = alloc_picture();
        if (!video_st.tmp_frame) {
            fprintf(stderr, "Could not allocate temporary picture\n");
            exit(1);
        }
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(video_st.st->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }

    av_dump_format(oc, 0, filename, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open '%s': %s\n", filename,
                    av_err2str(ret));
            exit(1);
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file: %s\n",
                av_err2str(ret));
        exit(1);
    }
}

void FrameWriter::close_video() {

    av_write_trailer(oc);

    avcodec_free_context(&video_st.enc);
    av_frame_free(&video_st.frame);
    av_frame_free(&video_st.tmp_frame);
    sws_freeContext(video_st.sws_ctx);
    swr_free(&video_st.swr_ctx);

    if (!(fmt->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_closep(&oc->pb);

    /* free the stream */
    avformat_free_context(oc);
    closed = true;
}

AVFrame *FrameWriter::alloc_picture() {
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame data.\n");
        exit(1);
    }

    return picture;
}

FrameWriter::~FrameWriter() {
    if(!closed)
        close_video();
}


int FrameWriter::write_frame() {
    int ret;

    // send the frame to the encoder
    ret = avcodec_send_frame(video_st.enc, video_st.frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame to the encoder: %s\n",
                av_err2str(ret));
        exit(1);
    }

    while (ret >= 0) {
        AVPacket pkt = { 0 };

        ret = avcodec_receive_packet(video_st.enc, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            fprintf(stderr, "Error encoding a frame: %s\n", av_err2str(ret));
            exit(1);
        }

        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(&pkt, video_st.enc->time_base, video_st.st->time_base);
        pkt.stream_index = video_st.st->index;

        /* Write the compressed frame to the media file. */
        ret = av_interleaved_write_frame(oc, &pkt);
        av_packet_unref(&pkt);
        if (ret < 0) {
            fprintf(stderr, "Error while writing output packet: %s\n", av_err2str(ret));
            exit(1);
        }
    }

    return ret == AVERROR_EOF ? 1 : 0;
}

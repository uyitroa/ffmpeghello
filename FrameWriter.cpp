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

    video_st.frame->pts = 0;
    video_st.next_pts = 1;

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
//    if(!closed)
//        close_video();
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
    video_st.frame->data[0][video_st.next_pts] = 255;
    video_st.frame->pts = video_st.next_pts++;

    return ret == AVERROR_EOF ? 1 : 0;
}


//TODO
//#define CALC_FFMPEG_VERSION(a,b,c) ( a<<16 | b<<8 | c )
//
//
//static const int OPENCV_NO_FRAMES_WRITTEN_CODE = 1000;
//
//static
//inline void _opencv_ffmpeg_av_packet_unref(AVPacket *pkt)
//{
//#if LIBAVCODEC_BUILD >= (LIBAVCODEC_VERSION_MICRO >= 100 \
//    ? CALC_FFMPEG_VERSION(55, 25, 100) : CALC_FFMPEG_VERSION(55, 16, 0))
//    av_packet_unref(pkt);
//#else
//    av_free_packet(pkt);
//#endif
//};
//
//static int icv_av_write_frame_FFMPEG( AVFormatContext * oc, AVStream * video_st,
//#if LIBAVCODEC_BUILD >= CALC_FFMPEG_VERSION(54, 1, 0)
//                                      uint8_t *, uint32_t,
//#else
//                                      uint8_t * outbuf, uint32_t outbuf_size,
//#endif
//                                      AVFrame * picture )
//{
//#if LIBAVFORMAT_BUILD > 4628
//    AVCodecContext * c = video_st->codec;
//#else
//    AVCodecContext * c = &(video_st->codec);
//#endif
//    int ret = OPENCV_NO_FRAMES_WRITTEN_CODE;
//
//#if LIBAVFORMAT_BUILD < CALC_FFMPEG_VERSION(57, 0, 0)
//    if (oc->oformat->flags & AVFMT_RAWPICTURE)
//    {
//        /* raw video case. The API will change slightly in the near
//           futur for that */
//        AVPacket pkt;
//        av_init_packet(&pkt);
//
//        pkt.flags |= PKT_FLAG_KEY;
//        pkt.stream_index= video_st->index;
//        pkt.data= (uint8_t *)picture;
//        pkt.size= sizeof(AVPicture);
//
//        ret = av_write_frame(oc, &pkt);
//    }
//    else
//#endif
//    {
//        /* encode the image */
//        AVPacket pkt;
//        av_init_packet(&pkt);
//#if LIBAVCODEC_BUILD >= CALC_FFMPEG_VERSION(54, 1, 0)
//        int got_output = 0;
//        pkt.data = NULL;
//        pkt.size = 0;
//        ret = avcodec_encode_video2(c, &pkt, picture, &got_output);
//        if (ret < 0)
//            ;
//        else if (got_output) {
//            if (pkt.pts != (int64_t)AV_NOPTS_VALUE)
//                pkt.pts = av_rescale_q(pkt.pts, c->time_base, video_st->time_base);
//            if (pkt.dts != (int64_t)AV_NOPTS_VALUE)
//                pkt.dts = av_rescale_q(pkt.dts, c->time_base, video_st->time_base);
//            if (pkt.duration)
//                pkt.duration = av_rescale_q(pkt.duration, c->time_base, video_st->time_base);
//            pkt.stream_index= video_st->index;
//            ret = av_write_frame(oc, &pkt);
//            _opencv_ffmpeg_av_packet_unref(&pkt);
//        }
//        else
//            ret = OPENCV_NO_FRAMES_WRITTEN_CODE;
//#else
//        int out_size = avcodec_encode_video(c, outbuf, outbuf_size, picture);
//        /* if zero size, it means the image was buffered */
//        if (out_size > 0) {
//#if LIBAVFORMAT_BUILD > 4752
//            if(c->coded_frame->pts != (int64_t)AV_NOPTS_VALUE)
//                pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, video_st->time_base);
//#else
//            pkt.pts = c->coded_frame->pts;
//#endif
//            if(c->coded_frame->key_frame)
//                pkt.flags |= PKT_FLAG_KEY;
//            pkt.stream_index= video_st->index;
//            pkt.data= outbuf;
//            pkt.size= out_size;
//
//            /* write the compressed frame in the media file */
//            ret = av_write_frame(oc, &pkt);
//        }
//#endif
//    }
//    return ret;
//}
//
//
///// write a frame with FFMPEG
//bool FrameWriter::writeFrame()
//{
//    // check parameters
//    if (pix_fmt == AV_PIX_FMT_BGR24) {
//        if (cn != 3) {
//            return false;
//        }
//    }
//    else if (pix_fmt == AV_PIX_FMT_GRAY8) {
//        if (cn != 1) {
//            return false;
//        }
//    }
//    else {
//        assert(false);
//    }
//
//    // typecast from opaque data type to implemented struct
//#if LIBAVFORMAT_BUILD > 4628
//    AVCodecContext *c = video_st->codec;
//#else
//    AVCodecContext *c = &(video_st->codec);
//#endif
//
//    // FFmpeg contains SIMD optimizations which can sometimes read data past
//    // the supplied input buffer.
//    // Related info: https://trac.ffmpeg.org/ticket/6763
//    // 1. To ensure that doesn't happen, we pad the step to a multiple of 32
//    // (that's the minimal alignment for which Valgrind doesn't raise any warnings).
//    // 2. (dataend - SIMD_SIZE) and (dataend + SIMD_SIZE) is from the same 4k page
//    const int CV_STEP_ALIGNMENT = 32;
//    const size_t CV_SIMD_SIZE = 32;
//    const size_t CV_PAGE_MASK = ~(4096 - 1);
//    const unsigned char* dataend = data + ((size_t)height * step);
//    if (step % CV_STEP_ALIGNMENT != 0 ||
//        (((size_t)dataend - CV_SIMD_SIZE) & CV_PAGE_MASK) != (((size_t)dataend + CV_SIMD_SIZE) & CV_PAGE_MASK))
//    {
//        int aligned_step = (step + CV_STEP_ALIGNMENT - 1) & ~(CV_STEP_ALIGNMENT - 1);
//
//        size_t new_size = (aligned_step * height + CV_SIMD_SIZE);
//
//        if (!aligned_input || aligned_input_size < new_size)
//        {
//            if (aligned_input)
//                av_freep(&aligned_input);
//            aligned_input_size = new_size;
//            aligned_input = (unsigned char*)av_mallocz(aligned_input_size);
//        }
//
//        if (origin == 1)
//            for( int y = 0; y < height; y++ )
//                memcpy(aligned_input + y*aligned_step, data + (height-1-y)*step, step);
//        else
//            for( int y = 0; y < height; y++ )
//                memcpy(aligned_input + y*aligned_step, data + y*step, step);
//
//        data = aligned_input;
//        step = aligned_step;
//    }
//
//    if ( c->pix_fmt != input_pix_fmt ) {
//        assert( input_picture );
//        // let input_picture point to the raw data buffer of 'image'
//        _opencv_ffmpeg_av_image_fill_arrays(input_picture, (uint8_t *) data,
//                       (AVPixelFormat)input_pix_fmt, width, height);
//        input_picture->linesize[0] = step;
//
//        if( !img_convert_ctx )
//        {
//            img_convert_ctx = sws_getContext(width,
//                                             height,
//                                             (AVPixelFormat)input_pix_fmt,
//                                             c->width,
//                                             c->height,
//                                             c->pix_fmt,
//                                             SWS_BICUBIC,
//                                             NULL, NULL, NULL);
//            if( !img_convert_ctx )
//                return false;
//        }
//
//        if ( sws_scale(img_convert_ctx, input_picture->data,
//                       input_picture->linesize, 0,
//                       height,
//                       picture->data, picture->linesize) < 0 )
//            return false;
//    }
//    else{
//        _opencv_ffmpeg_av_image_fill_arrays(picture, (uint8_t *) data,
//                       (AVPixelFormat)input_pix_fmt, width, height);
//        picture->linesize[0] = step;
//    }
//
//    picture->pts = frame_idx;
//    bool ret = icv_av_write_frame_FFMPEG( oc, video_st, outbuf, outbuf_size, picture) >= 0;
//    frame_idx++;
//
//    return ret;
//}

FrameWriter::FrameWriter() {

}


uint8_t *FrameWriter::getbuffer() {
    AVBufferRef *buffer = av_frame_get_plane_buffer(video_st.frame, 0);
    uint8_t *data = buffer->data;
//    free(buffer);
    return data;
}

int FrameWriter::getbuffersize() {
    AVBufferRef *buffer = av_frame_get_plane_buffer(video_st.frame, 0);
    int size = buffer->size;
//    free(buffer);
    return size;
}

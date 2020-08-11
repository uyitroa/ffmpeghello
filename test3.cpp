#include "mylib.cpp"

static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
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

static void fill_yuv_image(AVFrame *pict, int frame_index,
                           int width, int height)
{
    int x, y, i;

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = 10;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128;
            pict->data[2][y * pict->linesize[2] + x] = 64;
        }
    }
}

int main(void){
    MyArray a;
    AVFrame *picture = alloc_picture(AV_PIX_FMT_YUV420P, 1920, 1080);

    int ret = av_frame_get_buffer(picture, 0);
    av_frame_make_writable(picture);
    printf("%d\n", picture->channels);

    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame data.\n");
        exit(1);
    }

    fill_yuv_image(picture, 10, picture->width, picture->height);

    initialize_MyArray(&a, picture);

    print_MyArray(&a, 20);

//    av_frame_free(&picture);
    deallocate_MyArray(&a);
}
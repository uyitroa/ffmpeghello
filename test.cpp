#include "FrameWriter.h"


/* Prepare a dummy image. */
static void fill_yuv_image(AVFrame *pict, int frame_index,
                           int width, int height)
{
    int x, y, i;

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}


int main() {
    FrameWriter f("test.mp4", 1920, 1080, 60);
    f.open_video();
    for (int i = 0; i < 100; i++) {
        fill_yuv_image(f.video_st.frame, i, 1920, 1080);
        f.write_frame();
    }
    f.close_video();
}
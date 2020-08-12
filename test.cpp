#include "FrameWriter.h"


/* Prepare a dummy image. */
static void fill_yuv_image(AVFrame *pict, int frame_index,
                           int width, int height)
{
    for (int z = 0; z < 3; z++) {
        for (int y = frame_index; y < frame_index + 10; y++) {
            for (int x = frame_index; x < frame_index + 10; x++) {
                pict->data[z][y * pict->linesize[z] + x] = 255;
            }
        }
    }
}


int main() {
    FrameWriter f("test.mp4", 1920, 1080, 60);
    f.open_video();
    for (int i = 0; i < 1000; i++) {
        fill_yuv_image(f.video_st.frame, i, 1920, 1080);
        f.write_frame();
    }
    f.close_video();
}
#include <stdlib.h>
#include <stdio.h>

extern "C" {
#include <libavformat/avformat.h>
}

/* Structure defines a 1-dimensional strided array */
typedef struct{
    uint8_t *arr;
    long length;
} MyArray;

/* initialize the array with integers 0...length */
void initialize_MyArray(MyArray* a, AVFrame *picture){
    printf("HI3.1\n");
    AVBufferRef *buffer = av_frame_get_plane_buffer(picture, 0);
    printf("HI3.2\n");
    a->arr = buffer->data;
    printf("HI3.3\n");
    a->length = buffer->size;
    printf("HI3.4\n");
    av_free(buffer);
    printf("HI3.5\n");
}

/* free the memory when finished */
void deallocate_MyArray(MyArray* a){
    free(a->arr);
    a->arr = NULL;
}

/* tools to print the array */
char* stringify(MyArray* a, int nmax){
    char* output = (char*) malloc(nmax * 20);
    int pos = sprintf(&output[0], "[");

    for (int k=0; k < a->length && k < nmax; k++){
        pos += sprintf(&output[pos], " %d", a->arr[k]);
    }
    if(a->length > nmax)
        pos += sprintf(&output[pos], "...");
    sprintf(&output[pos], " ]");
    return output;
}

void print_MyArray(MyArray* a, int nmax){
    char* s = stringify(a, nmax);
    printf("%s", s);
    free(s);
}
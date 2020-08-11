#include <stdlib.h>
#include <stdio.h>

/* Structure defines a 1-dimensional strided array */
typedef struct{
    int* arr;
    long length;
} MyArray;

/* initialize the array with integers 0...length */
void initialize_MyArray(MyArray* a, long length){
    a->length = length;
    a->arr = (int*)malloc(length * sizeof(int));
    for(int i=0; i<length; i++){
        a->arr[i] = i;
    }
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
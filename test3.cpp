#include "mylib.cpp"

int main(void){
    MyArray a;
    initialize_MyArray(&a, 10);
    print_MyArray(&a, 10);
    deallocate_MyArray(&a);
}
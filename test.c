#include <stdio.h>
#include "threadpool.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

void hello(void* arg){
    char *s = arg;
    sleep(1);
    printf("%s\n", s);
    return;
}
int main(){
    threadpool *tp = threadpool_create(3);
    char *c;
    for(int i = 0; i < 10; i++){
        c = calloc(100, sizeof(char));
        snprintf(c,100, "hello %d", i);
        threadpool_submit(tp, hello, c);
        //sleep(1);
    }
    
    threadpool_destroy(tp, false);
    free(c);
    return 0;
}
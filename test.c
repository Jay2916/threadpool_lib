#include <stdio.h>
#include "threadpool.h"
#include <unistd.h>

void hello(void* arg){
    char *s = arg;
    printf("%s\n", s);
    return;
}
int main(){
    threadpool *tp = threadpool_create(3);
    threadpool_submit(tp, hello, "hello1");
    threadpool_submit(tp, hello, "hello2");
    threadpool_submit(tp, hello, "hello3");
    threadpool_submit(tp, hello, "hello4");
    threadpool_submit(tp, hello, "hello5");
    threadpool_submit(tp, hello, "hello6");
    threadpool_destory(tp);
    return 0;
}
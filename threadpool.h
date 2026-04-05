#include <stdbool.h>

typedef struct threadpool threadpool;
typedef struct task task;
typedef struct threadinfo threadinfo;
void *worker(void* arg);

threadpool *threadpool_create(int num_of_threads);
void threadpool_submit(threadpool *tp, void (*task_function)(void*), void *task_arg);
task threadpool_steal(threadinfo *thief_thread);
void threadpool_free(threadpool *tp);
void threadpool_wait(threadpool *tp);
void threadpool_destroy(threadpool *tp);

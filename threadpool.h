#include <stdbool.h>

typedef struct threadpool threadpool;
typedef struct job_t job_t;

void* worker_routine(void* arg);
threadpool* threadpool_create(int n);
void threadpool_submit(threadpool* tp, void (*function)(void*), void*);
void threadpool_wait(threadpool* tp);
void threadpool_waitAndDestroy(threadpool* tp);
void threadpool_destroy(threadpool* tp, bool immediate_shutdown);
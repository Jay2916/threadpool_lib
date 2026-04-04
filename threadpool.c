#include <pthread.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdbool.h>
#include <semaphore.h>
#include "../util/queue.h"
#include "threadpool.h"

#define MAX 100

struct job_t{
    void (*function)(void*);
    void *arg;
};

typedef enum STOP_COND{FALSE, IMMEDIATE, GRACEFULL} STOP_COND;
struct threadpool{
    STOP_COND stop;
    pthread_cond_t enqueue_cond;
    pthread_cond_t dequeue_cond;
    pthread_mutex_t mutex;
    int nthread;
    Queue *job_queue;
    pthread_t *thread;
};

void* worker_routine(void* arg){
    threadpool *tp = (threadpool*)arg;
    job_t job;
    
    while(true){
        pthread_mutex_lock(&tp->mutex);
        while(is_empty(tp->job_queue) && (tp->stop == FALSE) ){
            pthread_cond_wait(&tp->enqueue_cond, &tp->mutex);
        }
        if(tp->stop == IMMEDIATE || (tp->stop == GRACEFULL && is_empty(tp->job_queue))){
            pthread_mutex_unlock(&tp->mutex);
            break;
        }
        dequeue(tp->job_queue, &job);
        pthread_cond_signal(&tp->dequeue_cond);
        pthread_mutex_unlock(&tp->mutex);
        job.function(job.arg);
    }
    return NULL;
    
}

//mem leak: if one of the allocation call fails others have to be freed 
//assumed no failed allocations
threadpool* threadpool_create(int n){ 
    threadpool* tp = (threadpool*)malloc(sizeof(threadpool));
    if(tp == NULL){
        return NULL;
    }
    tp->nthread = 0;
    tp->stop = FALSE;
    pthread_cond_init(&tp->enqueue_cond, NULL);
    pthread_cond_init(&tp->dequeue_cond, NULL);
    tp->job_queue = create_queue(MAX, sizeof(job_t));
    tp->thread = (pthread_t*)calloc(n, sizeof(pthread_t));
    pthread_mutex_init(&tp->mutex, NULL);

    if(tp->job_queue == NULL || tp->thread == NULL){
        return NULL;
    }
    for(int i = 0; i < n; i++){
        tp->nthread++;
        pthread_create(tp->thread + i, NULL, worker_routine, (void*)tp);
        printf("Thread %d created.\n", i);
    }
    return tp;
}

//blocking submit => induces backpressure in queue
//can be made non blocking which returns error on Q_FULL
//some other ways to manage backpressure: unbounded queue(bad for general purpore threadpools, make user thread execute job when queue full)
void threadpool_submit(threadpool* tp, void (*function)(void*), void* arg){ 
    job_t job = {function, arg};

    pthread_mutex_lock(&tp->mutex);
    while(is_full(tp->job_queue) && tp->stop == FALSE){
        pthread_cond_wait(&tp->dequeue_cond, &tp->mutex);
    }
    enqueue(tp->job_queue, &job);
    pthread_cond_signal(&tp->enqueue_cond);
    pthread_mutex_unlock(&tp->mutex);
    

}

void threadpool_destroy(threadpool* tp, bool immediate_shutdown){ 
    pthread_mutex_lock(&tp->mutex);
    tp->stop = (immediate_shutdown)? IMMEDIATE : GRACEFULL;
    pthread_mutex_unlock(&tp->mutex);
    pthread_cond_broadcast(&tp->enqueue_cond);
    pthread_cond_broadcast(&tp->dequeue_cond);
    for(int i = 0;i<tp->nthread;i++){
        pthread_join(tp->thread[i], NULL);
        printf("Thread %d joined.\n",i);
    }
    pthread_mutex_destroy(&tp->mutex);
    pthread_cond_destroy(&tp->enqueue_cond);
    pthread_cond_destroy(&tp->dequeue_cond);
    destroy_queue(tp->job_queue);
    free(tp->thread);
    free(tp);
}


#include <pthread.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>
#include <stdio.h>
#include <semaphore.h>
#include "../util/queue.h"
#include "threadpool.h"

#define MAX 100
#define TIME 1

struct job_t{
    void (*function)(void*);
    void *arg;
};

struct threadpool{
    atomic_int stop;
    sem_t avaiable_jobs;
    pthread_mutex_t *mutex;
    int nthread;
    Queue *job_queue;
    pthread_t *thread;
};

void* worker_routine(void* arg){
    threadpool *tp = (threadpool*)arg;
    job_t job;
    int status;
    while(1){
        sem_wait(&tp->avaiable_jobs);
        pthread_mutex_lock(tp->mutex);
        status = dequeue(tp->job_queue, &job);
        pthread_mutex_unlock(tp->mutex);
        if(status == Q_OKAY){
            job.function(job.arg);
        }
        else if(status == Q_EMPTY && atomic_load(&tp->stop)){
            break;
        }
    }
    return NULL;
}
threadpool* threadpool_create(int n){
    threadpool* tp = (threadpool*)malloc(sizeof(threadpool));
    if(tp == NULL){
        return NULL;
    }
    atomic_store(&tp->stop, 0);
    tp->nthread = 0;
    sem_init(&tp->avaiable_jobs,0, 0);
    tp->job_queue = create_queue(MAX, sizeof(job_t));
    tp->thread = (pthread_t*)calloc(n, sizeof(pthread_t));
    tp->mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(tp->mutex, NULL);
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

void threadpool_submit(threadpool* tp, void (*function)(void*), void* arg){
    int status;
    job_t job = {function, arg};
    do{
        pthread_mutex_lock(tp->mutex);
        status = enqueue(tp->job_queue, &job);
        pthread_mutex_unlock(tp->mutex);
        if(status == Q_FULL) sleep(TIME);
    }while(status != Q_OKAY);
    sem_post(&tp->avaiable_jobs);
}

void threadpool_wait(threadpool* tp){

}

void threadpool_destory(threadpool* tp){
    atomic_store(&tp->stop, 1);
    for(int i = 0; i < tp->nthread;i++){
        sem_post(&tp->avaiable_jobs);
    }
    for(int i = 0;i<tp->nthread;i++){
        pthread_join(tp->thread[i], NULL);
        printf("Thread %d joined.\n",i);
    }
    sem_destroy(&tp->avaiable_jobs);
    pthread_mutex_destroy(tp->mutex);
    destroy_queue(tp->job_queue);
    free(tp->thread);
    free(tp->mutex);
    free(tp);
}


#include <pthread.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdbool.h>
#include <semaphore.h>
#include <time.h>
#include "../util/queue.h"
#include "threadpool.h"

#define MAX_TASKS 100

struct task{
        void (*function)(void*);
        void *arg;
};


struct threadpool{
        atomic_bool stop;
        pthread_mutex_t attr_mutex;
        pthread_cond_t empty_cond;
        unsigned int total_tasks;
        unsigned int max_threads;
        unsigned int next_index;
        threadinfo *thread_array;
};

struct threadinfo{
        pthread_t id;
        Queue *task_queue;
        pthread_mutex_t queue_mutex;
        threadpool *tp;
};

void *worker(void* arg){
        threadinfo *curr = arg;
        threadpool *tp = curr->tp;
        while(!atomic_load(&tp->stop)){
                task t = {NULL, NULL};
                pthread_mutex_lock(&curr->queue_mutex);
                if(is_empty(curr->task_queue)){ 
                        pthread_cond_signal(&tp->empty_cond);
                        pthread_mutex_unlock(&curr->queue_mutex);
                        t = threadpool_steal(curr); 
                }
                else{
                        dequeue_front(curr->task_queue, &t);
                        pthread_mutex_unlock(&curr->queue_mutex);
                }
                if(t.function != NULL){
                        t.function(t.arg);
                        pthread_mutex_lock(&tp->attr_mutex);
                        tp->total_tasks--;
                        pthread_mutex_unlock(&tp->attr_mutex);
                }      
        }
        return NULL;
}
threadpool *threadpool_create(int num_of_threads){
        threadpool *tp = malloc(sizeof(threadpool));
        if(tp == NULL){
                return NULL;
        }
        tp->next_index = 0;
        atomic_init(&tp->stop, false);
        pthread_mutex_init(&tp->attr_mutex, NULL);
        tp->max_threads = num_of_threads;
        tp->total_tasks = 0;
        pthread_cond_init(&tp->empty_cond, NULL);
        tp->thread_array = calloc(num_of_threads, sizeof(threadinfo));
        if(tp->thread_array == NULL){
                free(tp);
                return NULL;
        }        
        for(int i = 0; i < tp->max_threads; i++){
                threadinfo *curr = tp->thread_array + i;
                curr->tp = tp;
                
                curr->task_queue = create_queue(MAX_TASKS, sizeof(task));
                pthread_mutex_init(&curr->queue_mutex, NULL);     
        }
        for(int i = 0; i < tp->max_threads ;i++){
                threadinfo *curr = tp->thread_array + i;
                pthread_create(&curr->id, NULL, worker, curr);
                printf("Thread %d created.\n",i);
        }
        return tp;
}
void threadpool_submit(threadpool *tp, void (*task_function)(void*), void *task_arg){
        task t = {task_function, task_arg};
        threadinfo *thread_array = tp->thread_array;
        int i = tp->next_index;
        threadinfo *next_thread = thread_array + i;
        pthread_mutex_lock(&next_thread->queue_mutex);
        while(is_full(next_thread->task_queue)){
                pthread_mutex_unlock(&next_thread->queue_mutex);
                i = (i + 1) % tp->max_threads;
                pthread_mutex_lock(&next_thread->queue_mutex);
        }
        enqueue_rear(next_thread->task_queue, &t);
        pthread_mutex_lock(&tp->attr_mutex);
        tp->total_tasks++;
        pthread_mutex_unlock(&tp->attr_mutex);
        pthread_mutex_unlock(&next_thread->queue_mutex); 


}
task threadpool_steal(threadinfo *thief_thread){
        threadinfo *victim_thread;
        threadpool *tp = thief_thread->tp; 
        task t = {NULL, NULL};
        unsigned int seed = time(NULL) ^ (uintptr_t)thief_thread->id;
        int status;
        int counter = 0;
        int repeat = 1; // <= tp->max_threads
        do
        {       int idx = rand_r(&seed) % tp->max_threads;
                victim_thread = tp->thread_array + idx;
                pthread_mutex_lock(&victim_thread->queue_mutex);
                status = dequeue_front(victim_thread->task_queue, &t);
                pthread_mutex_unlock(&victim_thread->queue_mutex);
                counter++;
        }while(status != Q_OKAY && counter < repeat);
        return t;
}
void threadpool_free(threadpool *tp){
        threadinfo *temp = tp->thread_array;
        for(int i = 0; i < tp->max_threads; i++){
                threadinfo *curr = temp + i;
                destroy_queue(curr->task_queue);
                pthread_mutex_destroy(&curr->queue_mutex);
        }
         pthread_cond_destroy(&tp->empty_cond);
        pthread_mutex_destroy(&tp->attr_mutex);
        free(temp);
        free(tp);
}
void threadpool_wait(threadpool *tp){ //blocks till all the tasks are completed
        pthread_mutex_lock(&tp->attr_mutex);
        while(tp->total_tasks > 0){
                pthread_cond_wait(&tp->empty_cond, &tp->attr_mutex);
        }
        pthread_mutex_unlock(&tp->attr_mutex);
        printf("all tasks completed.\n");
        
}
void threadpool_destroy(threadpool *tp){
        atomic_store(&tp->stop, true);
        for(int i = 0;i < tp->max_threads; i++){
                threadinfo *curr = tp->thread_array + i;
                pthread_join(curr->id, NULL);
                printf("Thread %d joined.\n",i);
        }
        threadpool_free(tp);
}

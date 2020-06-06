#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

struct data_t {
    int id;
    int sleep;
};

void* thread(void *arg)
{
    struct data_t *data = arg;


    pthread_mutex_lock(&mutex);
    printf("Thread %d: waiting for release\n", data->id);

    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex); // unlocking for all other threads

    struct timeval tv;
    gettimeofday(&tv, NULL);    

    printf("Thread %d: doing some work for %d secs, started: %ld...\n", data->id, data->sleep, tv.tv_sec);
    sleep(data->sleep);
    gettimeofday(&tv, NULL);    
    printf("Thread %d: Bye, end at %ld\n", data->id, tv.tv_sec);
}

int main(void)
{

    struct data_t data[9];
    pthread_t ths[9];

    srand(time(NULL));

    for(int i = 0; i < 9; ++i)
    {
        data[i].id = i + 1;
        data[i].sleep = 1 + rand() % 6;

        pthread_create(ths + i, NULL, thread, data + i);
    }

    // give time for all threads to lock
    sleep(1);

    printf("Master: Now releasing the condition\n");

    pthread_cond_broadcast(&cond);

    for(int i = 0; i < 9; ++i)
        pthread_join(*(ths + i), NULL);

    return 0;
}
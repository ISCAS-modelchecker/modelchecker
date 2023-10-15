#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>

void *thread_start(void *arg){
    int arg_data = *((int *)arg);
    for(int i=0; i<10; i++){
        for(int j=0; j<50000000; j++);
        printf("receive arg %d = %d\n", arg_data, arg_data);
        pthread_exit(NULL);
    }
    
}

int main (void)
{
    pthread_t tid;
    int arg1 = 1;
    int arg2 = 2;
    int ret = pthread_create (&tid, NULL, thread_start, (void *)&arg1);
    if (ret) {
        perror ("pthread_create");
        exit(EXIT_FAILURE);
    }
    thread_start ((void *) &arg2);
    return 0;
}
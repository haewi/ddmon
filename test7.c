#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m4 = PTHREAD_MUTEX_INITIALIZER;
void* myfunc1(){
    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m2);
    printf("myfunc1\n");
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
}
void* myfunc2(){
    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&m3);
    printf("myfunc2\n");
    pthread_mutex_unlock(&m3);
    pthread_mutex_unlock(&m2);
}
void* myfunc3(){
    pthread_mutex_lock(&m3);
    pthread_mutex_lock(&m4);
    printf("myfunc3\n");
    pthread_mutex_unlock(&m4);
    pthread_mutex_unlock(&m3);
}
void* myfunc4(){
    pthread_mutex_lock(&m4);
    pthread_mutex_lock(&m1);
    printf("myfunc4\n");
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m4);
}
void main(){
    pthread_t thread1, thread2, thread3, thread4;
    pthread_create(&thread1, NULL, myfunc1, NULL);
    sleep(0.3);
    pthread_create(&thread2, NULL, myfunc2, NULL);
    sleep(0.3);
    pthread_create(&thread3, NULL, myfunc3, NULL);
    sleep(0.3);
    pthread_create(&thread4, NULL, myfunc4, NULL);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);
    printf("end\n");
}

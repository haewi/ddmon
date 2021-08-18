// deadlock prediction test case

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
	pthread_mutex_lock(&m3);
	pthread_mutex_lock(&m4);
	pthread_mutex_unlock(&m4);
	pthread_mutex_unlock(&m3);
	pthread_mutex_unlock(&m2);
	pthread_mutex_unlock(&m1);
}

void* myfunc2(){
	pthread_mutex_lock(&m2);
	pthread_mutex_lock(&m3);
	pthread_mutex_unlock(&m2);
	pthread_mutex_lock(&m4);
	pthread_mutex_lock(&m2);
	pthread_mutex_unlock(&m2);
	pthread_mutex_unlock(&m4);
	pthread_mutex_unlock(&m3);
}

void main(){
	printf(" %p - %p - %p - %p\n\n", &m1, &m2, &m3, &m4);
	pthread_t thread1, thread2;

	pthread_create(&thread1, NULL, myfunc1, NULL);
	sleep(2);
    
	pthread_create(&thread2, NULL, myfunc2, NULL);
    
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
}

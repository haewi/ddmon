#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m3 = PTHREAD_MUTEX_INITIALIZER;

void lock_num(int num);
void unlock_num(int num);

void *lock(){
	int lock1 = rand() % 3 + 1;
	int lock2 = 0;

	while((lock2 = rand() % 3 + 1) == lock1){}
	printf("%lu >> %d - %d\n", pthread_self(), lock1, lock2);

	lock_num(lock1);
	usleep(500);
	lock_num(lock3);

	usleep(500);

	unlock_num(lock1);
	unlock_num(lock2);
}

int main(){
	srand(time(0x0));

	printf("%p - %p - %p\n\n", &m1, &m2, &m3);

	pthread_t thread1, thread2;
	pthread_create(&thread1, NULL, lock, NULL);
	pthread_create(&thread2, NULL, lock, NULL);
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	
	printf("\n\n%d", errno);

	return 0;
}

void lock_num(int num){
	switch(num){
		case 1:
			pthread_mutex_lock(&m1);
			break;
		case 2:
			pthread_mutex_lock(&m2);
			break;
		case 3:
			pthread_mutex_lock(&m3);
			break;
	}
}

void unlock_num(int num){
	switch(num){
		case 1:
			pthread_mutex_unlock(&m1);
			break;
		case 2:
			pthread_mutex_unlock(&m2);
			break;
		case 3:
			pthread_mutex_unlock(&m3);
			break;
	}
}

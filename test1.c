#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *lock_unlock(void* m){
	m = (pthread_mutex_t *) m;
	pthread_mutex_lock(m);
	printf("thread1\n");
	pthread_mutex_unlock(m);
}

// simple thread locking and unlocking mutex
int main(){
	pthread_t thread1, thread2;
	pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

	pthread_create(&thread1, NULL, lock_unlock, (void*) &m);

	pthread_join(thread1, NULL);

	return 0;
}

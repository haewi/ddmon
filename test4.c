//Single cycle
//Program must not predict this cycle 

#include <stdio.h>
#include <pthread.h>

pthread_mutex_t m[5];

void *th(){
	pthread_mutex_lock(&m[0]);
	pthread_mutex_lock(&m[1]);
	pthread_mutex_lock(&m[2]);
	pthread_mutex_unlock(&m[2]);
	pthread_mutex_unlock(&m[1]);
	pthread_mutex_unlock(&m[0]);

	pthread_mutex_lock(&m[2]);
	pthread_mutex_lock(&m[0]);
	pthread_mutex_unlock(&m[0]);
	pthread_mutex_unlock(&m[2]);
	
}
int main(){
	pthread_t t[2];

	pthread_create(&t[0], 0x0, th, 0x0);

	pthread_join(t[0],0x0);

}


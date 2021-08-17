//Single cycle case 2


#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
pthread_mutex_t m[5];

void *th1(void* in){
	int i = *(int*)in;
	pthread_mutex_lock(&m[i]);
	pthread_mutex_lock(&m[i+1]);
	pthread_mutex_unlock(&m[i+1]);
	pthread_mutex_unlock(&m[i]);
}
void *th2(){
	pthread_mutex_lock(&m[2]);
	pthread_mutex_lock(&m[3]);
	pthread_mutex_unlock(&m[3]);
	pthread_mutex_unlock(&m[2]);
	
	pthread_mutex_lock(&m[3]);
	pthread_mutex_lock(&m[2]);
	pthread_mutex_unlock(&m[2]);
	pthread_mutex_unlock(&m[3]);
	

}
int main(){
	for(int i=0; i<5; i++){
		pthread_mutex_init(&m[i],0x0);
	}
	
	
	pthread_t th[5];
	for(int i=0; i<2; i++){
		pthread_create(&th[i],0x0, th1,(void*)&i);
		sleep(0.5);
	}
	
	sleep(0.5);
	pthread_create(&th[2],0x0,th2,0x0);
	
	for(int i=0; i<3; i++){
		pthread_join(th[i],0x0);
	}
	
	
}

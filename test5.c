#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int ex = 0;
// 0 : Not a deadlock but must be predicted
// 1 : Gatelock, Not a deadlock and must not be predicetd/
// 2 : Daedlock

pthread_mutex_t m[5];

void* myfunc1(){
	pthread_mutex_lock(&m[4]);
    	pthread_mutex_lock(&m[1]);
    	pthread_mutex_lock(&m[2]);
	if(ex==2)sleep(1);
	pthread_mutex_lock(&m[3]);
    	printf("myfunc1\n");
    	pthread_mutex_unlock(&m[3]);
    	pthread_mutex_unlock(&m[2]);
    	pthread_mutex_unlock(&m[1]);
	pthread_mutex_unlock(&m[4]);
}
void* myfunc2(){

	if(ex==1){
	pthread_mutex_lock(&m[4]);
    	pthread_mutex_lock(&m[3]);
    	pthread_mutex_lock(&m[1]);
    	pthread_mutex_lock(&m[2]);
    	printf("myfunc2\n");
   	pthread_mutex_unlock(&m[2]);
    	pthread_mutex_unlock(&m[1]);
    	pthread_mutex_unlock(&m[3]);
	pthread_mutex_unlock(&m[4]);
}

	else{
	pthread_mutex_lock(&m[3]);
	pthread_mutex_lock(&m[1]);
    	printf("myfunc2\n");
	pthread_mutex_unlock(&m[1]);
	pthread_mutex_unlock(&m[3]);
	}
}
void main(){
    pthread_t thread1, thread2;
    for(int i=0; i<5; i++){
    	pthread_mutex_init(&m[i],NULL);
    }
    pthread_create(&thread1, NULL, myfunc1, NULL);
    if(ex==0)sleep(1);
    pthread_create(&thread2, NULL, myfunc2, NULL);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    printf("end\n");
}

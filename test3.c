// not mine

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define PTHREAD_NUM 10

pthread_mutex_t first;
pthread_mutex_t second;

int cnt = 10;
int n = 0;

void * routine (){
    if( cnt % 2 == 0 ){
        pthread_mutex_lock( &first );
        pthread_mutex_lock( &second );
    } else {
        pthread_mutex_lock( &second );
        pthread_mutex_lock( &first );
    }

    cnt += 1;
    n += cnt * 10;

    printf("cnt %d n %d in routine \n", cnt, n);
    pthread_mutex_unlock(&first);
    pthread_mutex_unlock(&second);
}

int main(){
    pthread_t thread[PTHREAD_NUM];
    pthread_mutex_init(&first, 0x0);
    pthread_mutex_init(&second, 0x0);

    for(int i = 0 ; i < PTHREAD_NUM ; i++){
        if( pthread_create(&thread[i], 0x0, routine, 0x0) != 0 ){
            perror("pthread_create error\n");
            exit(1);
        }
    }

    for(int i = 0 ; i < PTHREAD_NUM ; i++){
        if( pthread_join(thread[i], 0x0) != 0 ){
            perror("pthread_join error\n");
            exit(1);
        }
    }

    printf("cnt %d n %d \n", cnt, n);
    exit(0);
}

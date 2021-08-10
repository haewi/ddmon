#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>

int read_bytes(int fd, void * a, int len);

int main(){
	// create .ddtrace fifo file
	if(mkfifo(".ddtrace", 0666)) {
		if(errno != EEXIST) {
			perror("fail to open fifo: tasks\n");
			exit(1);
		}
	}
	printf("ddchck - MADE channel\n");

		int ddtrace = open(".ddtrace", O_RDONLY | O_SYNC);
		printf("ddchck - opened channel\n");
		if(ddtrace < 0){
			fputs("[Error] ddchck - can't open .ddtrace\n", stderr);
		}

	while(1){
		printf("ddchck - in while loop\n");

		/*
		if(flock(ddtrace, LOCK_EX) != 0)
			fputs("[ERROR] ddchck - flock error\n", stderr);
		printf("ddchck - flock channel\n");
		*/
	
		int len=-1;
		int size=0;
		if((size=read_bytes(ddtrace, &len, sizeof(len))) != sizeof(len)){
			fputs("[ERROR] ffchck - reading int on channel\n", stderr);
			printf("sizzze: %d\n", size);
			close(ddtrace);
			exit(0);
		}
		printf("ddchck - Read int = %d\n", len);
		printf("size: %d\n", size);
	
		unsigned long int thread_id = 0;
		if((size = read_bytes(ddtrace, &thread_id, sizeof(thread_id))) != sizeof(thread_id)){
			fputs("[ERROR] ffchck - reading id on channel", stderr);
			printf(" %d\n", errno);
			close(ddtrace);
			exit(0);
		}
		printf("ddchck - Read id = %lu\n", thread_id);
		printf("size2: %d\n", size);
	
		/*
		pthread_mutex_t *mutex = (pthread_mutex_t*) malloc(sizeof(mutex));
		if(read_bytes(ddtrace, &mutex, sizeof(mutex)) != sizeof(mutex)){
			fputs("[ERROR] ffchck - reading on lock channel]n", stderr);
			close(ddtrace);
			exit(0);
		}
		printf("ddchck - Read lock\n");
	*/
		//flock(ddtrace, LOCK_UN);
	
		printf("-----------------------------------\n");
		printf("\t>> %d = %lu\n", len, thread_id);
		printf("-----------------------------------\n");
	}

	close(ddtrace);
	return 0;
}
/*
int read_bytes(int fd, void * a, size_t len) {
    char * s = (char *) a;
    //printf("read_bytes - %lu, %lu\n", len, sizeof(s));

    int i=0;
    for(i=0; i<len;) { // i : the length of successed message reading
        int b;
        b = read(fd, s + i, len - i);
        if(b == 0){
            break;
        }
        i += b;
        //printf("\n\ni:\t\t>%d\t\tb:\t\t%d\t\tlen\t\t%zu\t\tlen-i\t\t%lu\ts - %s\n\n", i, b, len, len-i, s);
    }
    // printf("read_bytes: -%s-\tlen-%zu-\ti-%d-\n", s, len, i);
    s[len] = '\0';
    //printf("the s[len]: %c\n", s[len]);
    //printf("\t> %s\tlen:\t%lu\n", s, strlen(s));

    //printf("read_bytes done\n");
    return i;
}*/

int read_bytes (int fd, void * a, int len) {
	char * s = (char *) a ;
	
	int i ;
	for (i = 0 ; i < len ; ) {
		int b ;
		b = read(fd, s + i, len - i) ;
		if (b == 0)
			break ;
		i += b ;
	}
	return i ; 
}

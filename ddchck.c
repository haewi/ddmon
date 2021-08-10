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

	/* ---------- Make & Open FIFO ----------*/	
	// create .ddtrace fifo file
	if(mkfifo(".ddtrace", 0666)) {
		if(errno != EEXIST) {
			perror("fail to open fifo: tasks\n");
			exit(1);
		}
	}
	printf("ddchck - MADE channel\n");

	int ddtrace = open(".ddtrace", O_RDWR | O_SYNC);
	printf("ddchck - opened channel\n");
	if(ddtrace < 0){
		fputs("[Error] ddchck - can't open .ddtrace\n", stderr);
	}
	
	while(1){

		int len=-1;
		long thread_id = -1;
		pthread_mutex_t *mutex = 0x0;

		// read info.
		if(read_bytes(ddtrace, &len, sizeof(len)) != sizeof(len)) {
			perror("[Error] ddchck - read int\n");
		}
		if(read_bytes(ddtrace, &thread_id, sizeof(thread_id)) != sizeof(thread_id)){
			perror("[Error] ddchck - read id\n");
		}
		if(read_bytes(ddtrace, &mutex, sizeof(mutex)) != sizeof(mutex)){
			perror("[Error] ddchck - read mutex\n");
		}

		if(len != -1 && thread_id != -1){
			printf("%d - %ld - %p\n", len, thread_id, mutex);
		}
	}
	close(ddtrace);

	return 0;
}

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


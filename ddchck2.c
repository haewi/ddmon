#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <pthread.h>

void
read_s (size_t len, char * data, int fd)
{
	size_t s;
	while(len > 0 && (s= read(fd, data, len)) > 0){
		data +=s;
		len -=s;
	}
}

int 
main()
{
	if(mkfifo(".ddtrace",0666)){
		if(errno != EEXIST){
			perror("fail to open fifo: ");
			exit(1);
		}
	}

	int fd = open(".ddtrace", O_RDWR | O_SYNC);

	while (1){
		int type = -1;
		pthread_mutex_t * addr = 0x0;
		long tid = -1;
		
		//flock(fd, LOCK_EX) ;
		read_s(sizeof(type), (char*)&type, fd);
		read_s(sizeof(addr), (char*)&addr, fd);
		read_s(sizeof(tid), (char*)&tid, fd);
		//flock(fd, LOCK_UN) ;

		if(type != -1 && addr != 0x0 && tid != -1){
			printf("%d %ld %p\n",type,tid,addr);
		}
	}
}

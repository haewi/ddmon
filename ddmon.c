#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>

int write_bytes(int fd, void * a, size_t len);

int pthread_mutex_lock(pthread_mutex_t *mutex){

	// function pointer for pthread_mutex_lock()
	int (*mutex_lock)(pthread_mutex_t *mutex);
	char *error;
	
	mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	if ((error = dlerror()) != 0x0)
		exit(1);

	// function pointer for pthread_self()
	pthread_t (*pthread_self)(void);
	pthread_self = dlsym(RTLD_NEXT, "pthread_self");
	if ((error = dlerror()) != 0x0)
		exit(1);

	// function return value
	int fd = mutex_lock(mutex);

	// print thread id, mutex address, function return value
	char buf[50];
	snprintf(buf, 50, "%lu >> pthread_mutex_lock(%p)=%d\n", pthread_self(), mutex, (int) fd);
	fputs(buf, stderr);
	fputs("\n", stderr);

	printf("now sending to channel\n");
	int ddtrace = open(".ddtrace", O_WRONLY | O_SYNC);
	if(ddtrace < 0)
		fputs("[Error] ddmon - can't open .ddtrace\n", stderr);
	fputs(" >> ddmon - open .ddtrace\n", stdout);
	
	if(flock(ddtrace, LOCK_EX) != 0)
		fputs("[Error] ddmon - flock error\n", stderr);
	fputs(" >> ddmon - flock .ddtrace\n", stdout);

	int len = 1;
	int size=-1;
	if((size =write_bytes(ddtrace, &len, sizeof(len))) != sizeof(len)){
		fputs("[ERROR] ddmon - writing int on channel\n", stderr);
		close(ddtrace);
		exit(0);
	}
	fputs(" >> ddmon - write int = len\n", stdout);
	printf("size: %d\n", size);
	
	unsigned long int id = pthread_self();
	if((size=write_bytes(ddtrace, &id, sizeof(id))) != sizeof(id)){
		fputs("[ERROR] ddmon - writing id on channel\n", stderr);
		close(ddtrace);
		exit(0);
	}
	printf(" >> ddmon - write id = %lu\n", id);
	printf("size: %d\n", size);
/*	
	if(write_bytes(ddtrace, &mutex, sizeof(mutex)) != sizeof(mutex)){
		fputs("[ERROR] ddmon - writing lock on channel\n", stderr);
		close(ddtrace);
		exit(0);
	}
	fputs(" >> ddmon - write lock = %p\n", stdout, mutex);
*/
	flock(ddtrace, LOCK_UN);
	printf("ddmon - unflock\n");
	close(ddtrace);
	printf("ddmon - close\n");

	return fd;
}

int write_bytes(int fd, void * a, size_t len) {
    	char * s = (char *) a;

    	int i=0;
    	while(i < len) { // i : the length of successed message writing
        	int b;
        	b = write(fd, s + i, len - i);
        	i += b;
    	}

    	return i;
}


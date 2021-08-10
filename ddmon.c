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
void write_s (size_t len, char * data, int fd);

int pthread_mutex_lock(pthread_mutex_t *mutex){

	// function pointer for pthread_mutex_lock()
	int (*mutex_lock)(pthread_mutex_t *mutex);
	pthread_t (*pthread_self)(void);

	char *error;
	
	mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	if ((error = dlerror()) != 0x0) {
		exit(1);
	}

	// function pointer for pthread_self()
	pthread_self = dlsym(RTLD_NEXT, "pthread_self");
	if ((error = dlerror()) != 0x0) {
		exit(1);
	}

	printf("thread_id: %lu\tmutex: %p\n", pthread_self(), mutex);

	/* ----------CHANNEL---------- */

	int ddtrace = open(".ddtrace", O_WRONLY | O_SYNC);
	if(ddtrace < 0)
		fputs("[Error] ddmon - can't open .ddtrace\n", stderr);
	if(flock(ddtrace, LOCK_EX) != 0)
		fputs("[Error] ddmon - flock error\n", stderr);
	printf(" >> ddmon - open & flock .ddtrace\n");

	/* ---------- Write ----------*/
	int len = 1;
	long thread_id = pthread_self();

	//write_s(sizeof(len), (char*)&len, ddtrace);
	write_bytes(ddtrace, &len, sizeof(len));

	//write_s(sizeof(thread_id), (char*)&thread_id, ddtrace);
	write_bytes(ddtrace, &thread_id, sizeof(thread_id));
	printf("ddmon - lock: %d - id: %lu - \n", len, thread_id);

	if(flock(ddtrace, LOCK_UN) != 0){
		fputs("[Error] ddmon - unflock error\n", stderr);
	}
	close(ddtrace);
	printf(" >> ddmon - close & unflock .ddtrace\n");

	return mutex_lock(mutex);















	
	/*
	int len = 1;
	write_s(sizeof(len), (char*)&len, ddtrace);

	unsigned long int id = pthread_self();
	write_s(sizeof(id), (char*)&id, ddtrace);
	
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
	
	
	if(write_bytes(ddtrace, &mutex, sizeof(mutex)) != sizeof(mutex)){
		fputs("[ERROR] ddmon - writing lock on channel\n", stderr);
		close(ddtrace);
		exit(0);
	}
	fputs(" >> ddmon - write lock = %p\n", stdout, mutex);

	flock(ddtrace, LOCK_UN);
	close(ddtrace);
	*/
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


void write_s (size_t len, char * data, int fd) {
	size_t s;
	while(len > 0 && (s = write(fd, data, len)) > 0){
		data += s;
		len -= s;
	}
}

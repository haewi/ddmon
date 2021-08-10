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
	int len = 1; // lock
	long thread_id = pthread_self();

	if(write_bytes(ddtrace, &len, sizeof(len)) != sizeof(len)){
		perror("[Error] ddchck - write int\n");
	}
	if(write_bytes(ddtrace, &thread_id, sizeof(thread_id)) != sizeof(thread_id)){
		perror("[Error] ddchck - write id\n");
	}
	if(write_bytes(ddtrace, &mutex, sizeof(mutex)) != sizeof(mutex)){
		perror("[Error] ddchck - write mutex\n");
	}

	printf("\tddmon - int: %d - id: %lu - lock: %p\n", len, thread_id, mutex);

	if(flock(ddtrace, LOCK_UN) != 0){
		fputs("[Error] ddmon - unflock error\n", stderr);
	}
	close(ddtrace);
	printf(" >> ddmon - close & unflock .ddtrace\n");

	return mutex_lock(mutex);

}

int pthread_mutex_unlock(pthread_mutex_t *mutex){
	// function pointer
	int (*mutex_unlock)(pthread_mutex_t *mutex);
	pthread_t (*pthread_self)(void);

	char * error;

	// call original function
	mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	if((error = dlerror()) != 0x0){
		exit(1);
	}

	pthread_self = dlsym(RTLD_NEXT, "pthread_self");
	if((error = dlerror()) != 0x0){
		exit(1);
	}
	
	/* ----------CHANNEL---------- */
	
	int ddtrace = open(".ddtrace", O_WRONLY | O_SYNC);
	if(ddtrace < 0)
		fputs("[Error] ddmon - can't open .ddtrace\n", stderr);
	if(flock(ddtrace, LOCK_EX) != 0)
		fputs("[Error] ddmon - flock error\n", stderr);
	printf(" >> ddmon - open & flock .ddtrace\n");

	/* ---------- Write ----------*/
	int len=0; // unlock
	long thread_id = pthread_self();

	write_bytes(ddtrace, &len, sizeof(len));
	write_bytes(ddtrace, &thread_id, sizeof(thread_id));
	write_bytes(ddtrace, &mutex, sizeof(mutex));

	printf("\tddmon - int: %d - id: %lu - lock: %p\n", len, thread_id, mutex);

	if(flock(ddtrace, LOCK_UN) != 0){
		fputs("[Error] ddmon - unflock error\n", stderr);
	}
	close(ddtrace);
	printf(" >> ddmon - close & unflock .ddtrace\n");

	return mutex_unlock(mutex);
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



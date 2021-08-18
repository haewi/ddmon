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
#include <execinfo.h>
#include <string.h>
#include <pthread.h>

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

int write_bytes(int fd, void * a, size_t len);
long get_line();

int pthread_mutex_lock(pthread_mutex_t *mutex){
	// function pointer for pthread_mutex_lock()
	int (*mutex_lock)(pthread_mutex_t *mutex);
	int (*mutex_unlock)(pthread_mutex_t *mutex);
	pthread_t (*pthread_self)(void);

	char *error;
	
	// get original functions
	mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	if ((error = dlerror()) != 0x0) {
		exit(1);
	}
	mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	if ((error = dlerror()) != 0x0) {
		exit(1);
	}
	pthread_self = dlsym(RTLD_NEXT, "pthread_self");
	if ((error = dlerror()) != 0x0) {
		exit(1);
	}

	// get backtrace info
	long line = get_line();

	//printf("thread_id: %lu\tmutex: %p\n", pthread_self(), mutex);

	/* ----------CHANNEL---------- */

	int ddtrace = open(".ddtrace", O_WRONLY | O_SYNC);
	if(ddtrace < 0)
		fputs("[Error] ddmon - can't open .ddtrace\n", stderr);

	/* ---------- Write ----------*/
	int len = 1; // lock
	long thread_id = pthread_self();

	mutex_lock(&m);
	if(write_bytes(ddtrace, &len, sizeof(len)) != sizeof(len)){
		perror("[Error] ddmon - write int\n");
	}
	if(write_bytes(ddtrace, &thread_id, sizeof(thread_id)) != sizeof(thread_id)){
		perror("[Error] ddmon - write id\n");
	}
	if(write_bytes(ddtrace, &mutex, sizeof(mutex)) != sizeof(mutex)){
		perror("[Error] ddmon - write mutex\n");
	}
	if(write_bytes(ddtrace, &line, sizeof(line)) != sizeof(line)){
		perror("[Error] ddmon - write line\n");
	}
	mutex_unlock(&m);
	printf("\tddmon - int: %d - id: %lu - lock: %p - line: %li\n", len, thread_id, mutex, line);

	close(ddtrace);

	return mutex_lock(mutex);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex){
	// function pointer
	int (*mutex_lock)(pthread_mutex_t *mutex);
	int (*mutex_unlock)(pthread_mutex_t *mutex);
	pthread_t (*pthread_self)(void);

	char * error;

	// call original function
	mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	if ((error = dlerror()) != 0x0) {
		exit(1);
	}
	mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	if((error = dlerror()) != 0x0){
		exit(1);
	}

	pthread_self = dlsym(RTLD_NEXT, "pthread_self");
	if((error = dlerror()) != 0x0){
		exit(1);
	}
	
	// get backtrace info
	long line = get_line();

	/* ----------CHANNEL---------- */
	
	int ddtrace = open(".ddtrace", O_WRONLY | O_SYNC);
	if(ddtrace < 0)
		fputs("[Error] ddmon - can't open .ddtrace\n", stderr);
	if(flock(ddtrace, LOCK_EX) != 0)
		fputs("[Error] ddmon - flock error\n", stderr);
	//printf(" >> ddmon - open & flock .ddtrace\n");

	/* ---------- Write ----------*/
	int len=0; // unlock
	long thread_id = pthread_self();

	mutex_lock(&m);
	if(write_bytes(ddtrace, &len, sizeof(len)) != sizeof(len)){
		perror("[Error] ddchck - write int\n");
	}
	if(write_bytes(ddtrace, &thread_id, sizeof(thread_id)) != sizeof(thread_id)){
		perror("[Error] ddchck - write id\n");
	}
	if(write_bytes(ddtrace, &mutex, sizeof(mutex)) != sizeof(mutex)){
		perror("[Error] ddchck - write mutex\n");
	}
	if(write_bytes(ddtrace, &line, sizeof(line)) != sizeof(line)){
		perror("[Error] ddchck - write line\n");
	}
	mutex_unlock(&m);

	printf("\tddmon - int: %d - id: %lu - lock: %p - line: %li\n", len, thread_id, mutex, line);

	int fd = mutex_unlock(mutex);

	if(flock(ddtrace, LOCK_UN) != 0){
		fputs("[Error] ddmon - unflock error\n", stderr);
	}
	close(ddtrace);
	//printf(" >> ddmon - close & unflock .ddtrace\n");

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

long get_line(){
	int nptrs;
	void *buffer[10];
	char ** str;

	nptrs = backtrace(buffer, 10);

	str = backtrace_symbols(buffer, nptrs);
	if (str == NULL) {
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}
	//printf("%s\n", str[2]);

	// find the address
	int index = -1;
	for(int i=0; i<strlen(str[2]); i++){
		if(str[2][i] == 43){ // '+'
			index = i;
			break;
		}
	}
	if(index < 0){
		printf("[Error] ddmon - backtrace\n");
		exit(0);
	}

	char* address = (char*) malloc(sizeof(char)*50);
	while(str[2][index+1] != 41){ // ')'
		index++;
		address = strncat(address, &str[2][index], 1);
	}
	long line = strtol(address, NULL, 16);
	//printf("%s\n", str[2]);

	free(str);
	free(address);
	return line;
}


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <execinfo.h>

int pthread_mutex_lock(pthread_mutex_t *mutex){
	int (*mutex_lock)(pthread_mutex_t *mutex);
	char *error;
	
	mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	if ((error = dlerror()) != 0x0)
		exit(1);

	int fd = mutex_lock(mutex);

	char buf[50];
	snprintf(buf, 50, "pthread_mutex_lock(%p)=%d\n", mutex, (int) fd);
	fputs(buf, stderr);

	return fd;
}

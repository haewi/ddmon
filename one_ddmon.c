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

#define NN 10 // maximum mutex number
#define TN 10 // maximum thread number

typedef enum {
	White, Gray, Black
} color;

typedef struct Node{
	pthread_mutex_t * mutex;
	long thread_id; // 0 means 'unknown'
	color col;
	struct Node ** edges;
	long line; //for mutex nodes, 0 means 'unknown'
	long lines[NN]; // for thread nodes, the line info that called the mutexes in the edges variable
} node;
/*
	RULE
	the edges variable in the node structure points to the destination node (the edge is saved in the starting node)
*/

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
node ** nodes = 0x0;
char* filename = "test2";

int write_bytes(int fd, void * a, size_t len);
long get_line();
void init_graph();
void make_thread_nodes(node ** nodes, long thread_id);
void unknown2known(node ** nodes, long thread_id);
void add_info(node ** nodes, pthread_mutex_t * mutex, long thread_id, long line);
void delete_info(node ** nodes, pthread_mutex_t * mutex, long thread_id, long line);
void init_node(node* n);
void release_node(node* n);
void print_graph(node ** nodes);
void dfs(node ** nodes, char* filename);
void dfs_visit(node ** nodes, node * n, int * cycle);
void print_cycle(node ** nodes, char* filename);
char* d2hex(long line);
char* addr2line(long line, char* filename);

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
	//printf("thread_id: %lu\tmutex: %p\n", pthread_self(), mutex);

	long line = get_line(); // get backtrace info
	long thread_id = pthread_self();

	mutex_lock(&m);
	// ---- make graph ----
	// initialize nodes if needed
	if(nodes == 0x0){
		init_graph();
	}
	// make thread nodes
	make_thread_nodes(nodes, thread_id);
	
	// make all unknown nodes in the thread node to known
	unknown2known(nodes, thread_id);
	
	// handle info.
	add_info(nodes, mutex, thread_id, line);
	
	// print graph
	print_graph(nodes);

	// detect cycle
	printf(" -------------------------- cycle detecting... --------------------------\n");
	dfs(nodes, filename);
	mutex_unlock(&m);

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
	
	long line = get_line(); // get backtrace info
	long thread_id = pthread_self();

	mutex_lock(&m);
	// ---- make graph ----
	// initialize nodes if needed
	if(nodes == 0x0){
		init_graph();
	}
	// make thread nodes
	make_thread_nodes(nodes, thread_id);
	
	// make all unknown nodes in the thread node to known
	unknown2known(nodes, thread_id);

	// handle info.
	delete_info(nodes, mutex, thread_id, line);

	// print graph
	print_graph(nodes);

	// detect cycle
	printf(" -------------------------- cycle detecting... --------------------------\n");
	dfs(nodes, filename);
	mutex_unlock(&m);

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

void init_graph(){
	nodes = (node**) malloc((NN+TN)*sizeof(node*));
	for(int i=0; i< NN+TN; i++){
		nodes[i] = 0x0;
	}
}

void make_thread_nodes(node ** nodes, long thread_id){
	int none = -1;
	for(int i=NN; i< NN+TN; i++){
		if(nodes[i] != 0x0 && nodes[i]->thread_id == thread_id){ // thread node already exist
			//printf("found = nodes[%d]->thread_id = %lu\n", i, nodes[i]->thread_id);
			none = i;
			break;
		}
	}
	if(none == -1){ // no thread node
		for(int i=NN; i < NN+TN; i++){ // for all thread nodes
			if(nodes[i] == 0x0) { // find empty thread node
				//printf("found empty thread node: %d\n", i);
				nodes[i] = (node*) malloc(sizeof(node));
				init_node(nodes[i]);
				nodes[i]->thread_id = thread_id; // and fill it
				break;
			}
		}
	}
}

void unknown2known(node ** nodes, long thread_id){
	for(int i=NN; i< NN+TN; i++){
		if(nodes[i] !=  0x0 && nodes[i]->thread_id == thread_id){ // find thread node
			for(int j=0; j<NN; j++){ // for all nodes the thread node is pointing
				if(nodes[i]->edges[j] != 0x0){
					nodes[i]->edges[j]->thread_id = thread_id; // update id info
					nodes[i]->edges[j]->line = nodes[i]->lines[j]; // update line info
				}
			}
			break;
		}
	}
}

void add_info(node ** nodes, pthread_mutex_t * mutex, long thread_id, long line){
	int found=-1;
	for(int i=0; i<NN; i++){ 
		if(nodes[i] != 0x0 && nodes[i]->mutex == mutex){ // if found node
			found=i; // found the node
			break;
		}
	}

	if(found == -1){ // no node with the same mutex info.
		printf(">> no node\n");

		// make new node
		node *tmp = (node*) malloc(sizeof(node));
		init_node(tmp);
		tmp->mutex = mutex;
		tmp->thread_id = thread_id;
		tmp->line = line;

		// add node to nodes variable
		for(int i=0; i<NN; i++){
			if(nodes[i] == 0x0) { // a empty spot in the nodes variable
				//printf("node[%d] = 0x0\n", i);
				nodes[i] = tmp;
				found = i;
				//printf("added node on nodes[%d]\n", i);
				break;
			}
		}
	}
	else {
		printf(">> found node\n");
	}

	// add edges
	printf(">>> add edges\n");
	for(int i=0; i < NN+TN ; i++){ // for all nodes
		if(nodes[i] == 0x0 || i == found) {
			continue;
		}
		// existing node & not itself

		if(nodes[i]->thread_id == thread_id && i != found){ // with the same thread id
			//printf("nodes[%d]->thread_id = %lu\n", i, nodes[i]->thread_id);
			for(int j=0; j<NN; j++){
				if(nodes[i]->edges[j] == 0x0){ // add edge
					//printf("nodes[%d]->edges[%d]\n", i, j);
					nodes[i]->edges[j] = nodes[found];
					if(i >= NN){ // thread node일때
						nodes[i]->lines[j] = line;
					}
					break;
				}
			}
		}
	}
}

void delete_info(node ** nodes, pthread_mutex_t * mutex, long thread_id, long line){
	// 1.
	int found=-1;
	for(int i=0; i< NN; i++){
		if(nodes[i] == 0x0) continue;
		//printf("nodes[%d]->mutex: %p\n", i, nodes[i]->mutex);
		if(nodes[i]!=0x0 && nodes[i]->mutex == mutex){ // find node with the same mutex info
			found = i;
			break;
		}
	}
	if(found == -1){
		perror("[ERROR] ddchck - trying to unlock not held lock\n");
		exit(1);
	}
	//printf("found: %d\n", found);

	// 2.

	printf("-----------------delete same thread edges-------------------\n");
	for(int i=0; i< NN+TN; i++){
		if(nodes[i]!=0x0 && nodes[i]->thread_id == thread_id){
			for(int j=0; j<NN; j++){
				if(nodes[i]->edges[j] == nodes[found]){
					//printf("nodes[found]: %p - mutex: %p - id: %lu\n", nodes[found], nodes[found]->mutex, nodes[found]->thread_id);
					//printf("nodes[i]->edges[j]: %p\n", nodes[i]->edges[j]);
					//printf("nodes[found]: %p - mutex: %p - id: %lu\n", nodes[found], nodes[found]->mutex, nodes[found]->thread_id);
					nodes[i]->edges[j] = 0x0;
					break;
				}
			}
		}
	}
	//nodes[found]->mutex = mutex;
	//printf(" <> nodes[found]->mutex: %p - id: %lu\n", nodes[found]->mutex, nodes[found]->thread_id);

	// 3.
	int exist = 0;
	for(int i=NN; i< NN+TN; i++){
		if(nodes[i] != 0x0){ // other thread id nodes
			for(int j=0; j<NN; j++){
				if(nodes[i]->edges[j] != 0x0 && nodes[i]->edges[j] == nodes[found]){
					exist = 1; // edge exists
					//printf("node[%d]->thread_id = %lu\n", i, nodes[i]->thread_id);
					break;
				}
			}
		}
	}
	
	if(exist == 0){
		release_node(nodes[found]);
		nodes[found]=0x0;
	}
	else{
		printf("edges exists\n");
		//printf("nodes[found]->thread_id = %lu\n", nodes[found]->thread_id);
		nodes[found]->thread_id = 0;
		nodes[found]->line = 0;
		//printf("nodes[found]->thread_id = %lu\n", nodes[found]->thread_id);
		for(int i=0; i<NN; i++){ // delete edges in the node
			if(nodes[found]->edges[i] != 0x0){
				//printf("1. nodex[%d]->edges[%d]->mutex = %p\n", found, i, nodes[found]->edges[i]->mutex);
				//pthread_mutex_t * t = nodes[found]->edges[i]->mutex;
				//nodes[found]->edges[i]->mutex = t;
				nodes[found]->edges[i] = 0x0;
				//printf("2. nodex[%d]->edges[%d]->mutex = %p\n", found, i, nodes[found]->edges[i]->mutex);
			}
		}
	}
}

void init_node(node* n) {
	n->mutex = 0x0;
	n->thread_id = 0;
	n->col = White;
	n->line = 0;

	n->edges = (node**) malloc(NN*sizeof(node*));
	for(int i=0; i<NN; i++){
		n->edges[i] = 0x0;
		n->lines[i] = 0;
	}
}

void release_node(node* n){
	free(n->edges);
	free(n);
	n = NULL;
}

void print_graph(node ** nodes) {
	printf("\n|----------------------------[node info.]------------------------------|\n|\n");
	printf("|\tnodes' address - [");
	for(int i=0; i<NN; i++){
		if(nodes[i] != 0x0){
			printf("%p ", nodes[i]);
		}
	}
	printf("]\n|\n");
	printf("|\n| --- mutex nodes info. ---\n|\n");
	for(int i=0; i<NN; i++){
		if(nodes[i] == 0x0) continue;
		printf("| - node[%d]=%p -> mutex: %p - id: %lu - line: %li\n", i, nodes[i], nodes[i]->mutex, nodes[i]->thread_id, nodes[i]->line);
		for(int j=0; j<NN; j++){
			if(nodes[i]->edges[j] != 0x0){
				printf("|   nodes[%d]->edges[%d] = %p\n", i, j, nodes[i]->edges[j]);
			}
		}
		printf("| \t --- edge printing done\n|\n");
	}
	printf("| --- thread nodes info. --- \n|\n");
	for(int i=NN; i< NN+TN; i++){
		if(nodes[i] == 0x0) continue;
		
		printf("| - node[%d]=%p -> id: %lu\n", i, nodes[i], nodes[i]->thread_id);
		for(int j=0; j<NN; j++){
			if(nodes[i]->edges[j] != 0x0){
				printf("|   nodes[%d]->edges[%d] = %p - line: %li\n", i, j, nodes[i]->edges[j], nodes[i]->lines[j]);
			}
		}
		printf("| \t --- edge printing done\n|\n");
	}
	
	printf("|-----------------------------------------------------------------------|\n\n");
}

/*
   1. make all color white
   2. for all nodes start dfs_visit
*/
void dfs(node ** nodes, char* filename){

	// make color white
	for(int i=0; i<NN; i++){
		if(nodes[i] == 0x0) continue;
		nodes[i]->col = White;
	}

	// start dfs_visit
	int cycle=0;
	for(int i=0; i<NN+TN; i++){
		if(nodes[i] == 0x0) continue;
		if(nodes[i]->col == White){
			dfs_visit(nodes, nodes[i], &cycle);
		}
		if(cycle == 1){
			printf("\n\t\t-----------[CYCLE!!!!]-------------\n");
			print_cycle(nodes, filename);
			printf("\n-------------------------------------------------------------------------\n\n");
			exit(0);
			return;
		}
	}

	printf("\n\t\t\t\tno cycle\n\n");
	printf("-------------------------------------------------------------------------\n\n");
}

/*
   1. make node's color Gray
   2. for all adjacent nodes,
   	2-1) if color is White,
		start dfs_visit
	2-2) if color is Gray,
		stop dfs_visit & alert a cycle deadlock
   3. make color Black
*/
void dfs_visit(node ** nodes, node * n, int * cycle){
	//printf("dfs_visit - %p\n", n);
	n->col = Gray;
	
	for(int i=0; i< NN; i++){
		if(n->edges[i] == 0x0) continue;
		//printf("n->edges[%d]: %p\n", i, n->edges[i]);
		if(n->edges[i]->col == Gray){
			//printf("cycle: %p\n", n);
			*cycle = 1;
			
			return;
		}
		if(n->edges[i]->col == White){
			dfs_visit(nodes, n->edges[i], cycle);
		}
	}
	if(*cycle == 1){
		return;
	}

	n->col = Black;
}

void print_cycle(node ** nodes, char* filename){
	printf("\t\t-- Threads related to the Deadlock --\n");
/*	for(int i=0; i<NN; i++){
		if(nodes[i] == 0x0) continue;
		if(nodes[i]->col == Gray){
			printf("\tthread id: %lu - mutex address: %p - line number: \n", nodes[i]->thread_id, nodes[i]->mutex);
			printf("%s\n", d2hex(nodes[i]->line));
		}
	}*/

	for(int i=NN; i<NN+TN; i++){ // for all thread nodes
		if(nodes[i] == 0x0) continue;
		for(int j=0; j<NN; j++){ // for all edges
			if(nodes[i]->edges[j] == 0x0) continue;
			if(nodes[i]->edges[j]->col == Gray){
				//printf("nodes[%d]->lines[%d] = %li\n", i, j, nodes[i]->lines[j]);
				printf("thread id: %lu - mutex address: %p - line number: %s\n", nodes[i]->edges[j]->thread_id, nodes[i]->edges[j]->mutex, addr2line(nodes[i]->lines[j], filename) );
			}
		}
	}
}

char* d2hex(long line){
	char* t_hex = (char*) malloc(sizeof(char)*100);
	char* hex = (char*) malloc(sizeof(char)*100);
	long quotient = line;
	int temp, i=1;

	while(quotient!=0) {
		temp = quotient % 16;
		//To convert integer into character
		if( temp < 10)
		           temp =temp + 48; else
		         temp = temp + 55;
		t_hex[i++]= temp;
		quotient = quotient / 16;
	}
	hex[0]='0';
	hex[1]='x';

	for(int j=0; j<i; j++){
		hex[j+2] = t_hex[i-1-j];
	}
	free(t_hex);
	return hex;
}

char* addr2line(long line, char* filename){
	char* hex = d2hex(line);
	FILE *fp;
	int status;
	char path[200];

	char* command = (char*) malloc(sizeof(char)*100);
	char* addr = "addr2line -e ";
	command = strcat(strcat(strcat(strcat(command, addr), filename), " "), hex);
	//printf("command: %s\n", command);
	
	// execute addr2line
	fp = popen(command, "r");
	if(fp == 0x0){
		perror("[Error] addr2line popen\n"); 
	}
	
	// get result
	if(fgets(path, 200, fp) == NULL){
		perror("[Error] addr2line fgets\n");
	}

	free(command);
	
	// parse line number
	char* li = (char*) malloc(sizeof(char)*10);
	for(int i=0; i<strlen(path); i++){
		if(path[i] == ':'){
			for(int j=i+1; j<strlen(path)-1; j++){
				li[j-i-1] = path[j];
			}
			break;
		}
	}
	li[strlen(li)-1] -= 1;
	//printf("li: %s strlen(li)=%ld\n", li, strlen(li));

	return li;
}











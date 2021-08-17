#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
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
	long threads[NN];
	int valid[NN];
} node;
/*
	RULE
	the edges variable in the node structure points to the destination node (the edge is saved in the starting node)
 */

pthread_mutex_t graph = PTHREAD_MUTEX_INITIALIZER;

int read_bytes(int fd, void * a, int len);
void init_node(node* n);
void release_node(node* n);
void print_graph(node ** nodes);
void dfs(node ** nodes, char* filename);
void dfs_visit(node ** nodes, node * n, int * cycle);
void print_cycle(node ** nodes, char* filename);
char* d2hex(long line);
char* addr2line(long line, char* filename);
void make_thread_nodes(node ** nodes, long thread_id);
void unknown2known(node ** nodes, long thread_id);
void lock(node ** nodes, pthread_mutex_t * mutex, long thread_id, long line);
void unlock(node ** nodes, pthread_mutex_t * mutex, long thread_id, long line);

int main(int argc, char *argv[]){

	/* ---------- Make & Open FIFO ----------*/	
	// create .ddtrace fifo file
	if(mkfifo(".ddtrace", 0666)) {
		if(errno != EEXIST) {
			perror("fail to open fifo: tasks\n");
			exit(1);
		}
	}
	//printf("ddchck - MADE channel\n");

	int ddtrace = open(".ddtrace", O_RDWR | O_SYNC);
	//printf("ddchck - opened channel\n");
	if(ddtrace < 0){
		fputs("[Error] ddchck - can't open .ddtrace\n", stderr);
	}

	/* ---------- parse commandline argument ----------*/
	if(argc != 2) {
		perror("[Error] no file input\n");
		exit(1);
	}
	/*
	char * first = "LD_PRELOAD=\"./k_ddmon.so\" ./";
	char * command = (char*) malloc((strlen(argv[1]) + strlen(first)) * sizeof(char));
	strcat(command, first);
	strcat(command, argv[1]);

	pid_t child = fork();
	if(child == 0){
		//char* myargs[3];
		//myargs[0] = strdup("LD_PRELOAD=\"ddmon.so\"");
		//myargs[1] = strdup(argv[1]);
		//myargs[2] = NULL;
		//if(execl(command, "", NULL) == -1){
		//	perror("[Error] ddchck - execl\n");
		//	exit(1);
		//}
		//execvp(myargs[0], myargs);
		if(system(command) == -1){
			perror("system(command) error\n");
			n->threads[i] = 0;
		}
	}

	free(command);*/
	
	// init nodes (if mutex not held, it will be 0x0)
	node ** nodes = (node**) malloc((NN + TN)*sizeof(node*));
	for(int i = 0 ; i< NN+TN; i++){ // nodes for mutex
		nodes[i] = 0x0;
	}
	
	while(1){

		int len=-1;
		long thread_id = 0;
		pthread_mutex_t *mutex = 0x0;
		long line =0;

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
		if(read_bytes(ddtrace, &line, sizeof(line)) != sizeof(line)){
			perror("[Error] ddchck - read line\n");
		}
		

		printf("\t| %d - %ld - %p - %li|\n\n", len, thread_id, mutex, line);

		// make thread nodes
		pthread_mutex_lock(&graph);
		make_thread_nodes(nodes, thread_id);
		pthread_mutex_unlock(&graph);

		// make all unknown nodes in the thread node to known
		pthread_mutex_lock(&graph);
		unknown2known(nodes, thread_id);
		pthread_mutex_unlock(&graph);

		// handle info.
		if(len == 1) { // lock executed
			printf("> lock executed\n");
			pthread_mutex_lock(&graph);
			lock(nodes, mutex, thread_id, line);
			pthread_mutex_unlock(&graph);
			
		} // lock executed
		else{
			printf("> unlock executed\n");
			unlock(nodes, mutex, thread_id, line);
		}
		//printf("done\n");

		pthread_mutex_lock(&graph);
		print_graph(nodes);
		pthread_mutex_unlock(&graph);

		printf(" -------------------------- cycle detecting... --------------------------\n");
		pthread_mutex_lock(&graph);
		dfs(nodes, argv[1]);
		pthread_mutex_unlock(&graph);


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

void init_node(node* n) {
	n->mutex = 0x0;
	n->thread_id = 0;
	n->col = White;
	n->line = 0;

	n->edges = (node**) malloc(NN*sizeof(node*));
	for(int i=0; i<NN; i++){
		n->edges[i] = 0x0;
		n->lines[i] = 0;
		n->threads[i] = 0;
		n->valid[i] = 0;
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
				printf("|   nodes[%d]->edges[%d] = %p - thread: %li - valid: %d\n", i, j, nodes[i]->edges[j], nodes[i]->threads[j], nodes[i]->valid[j]);
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
				printf("|   nodes[%d]->edges[%d] = %p - threads: %li - line: %li - valid: %d\n", i, j, nodes[i]->edges[j], nodes[i]->threads[j],  nodes[i]->lines[j], nodes[i]->valid[j]);
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
				nodes[i]->line = 1;
				break;
			}
		}
	}
}

void unknown2known(node ** nodes, long thread_id){
	for(int i=NN; i< NN+TN; i++){
		if(nodes[i] !=  0x0 && nodes[i]->thread_id == thread_id){ // find thread node
			for(int j=0; j<NN; j++){ // for all nodes the thread node is pointing
				if(nodes[i]->edges[j] != 0x0 && nodes[i]->valid[j] == 1 && nodes[i]->edges[j]->line == 0){
					nodes[i]->edges[j]->thread_id = thread_id; // update id info
					nodes[i]->edges[j]->line = nodes[i]->lines[j]; // update line info
				}
			}
			break;
		}
	}
}

/*
 	1. find node with the same mutex info
	   - found -> add edges = nodes with the same thread id -> found node
	   - not found -> add node & add edges = nodes with the same thread id -> made node
	2. adjust thread id of all nodes with the same thread id
 */
void lock(node ** nodes, pthread_mutex_t * mutex, long thread_id, long line){
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
		if(nodes[found]->line != 0){ // 0 means that another thread wanted it
			int taken = 0;
			for(int i=NN; i<NN+TN; i++){
				if(nodes[i] != 0x0 && nodes[i]->thread_id != thread_id) { // other thread nodes
					for(int j=0; j<NN; j++){
						// have valid edges pointing to the found node
						if(nodes[i]->valid[j] == 1 && nodes[i]->edges[j] != 0x0 && nodes[i]->edges[j] == nodes[found]){
							taken = 1;
							break;
						}
					}
				}
			}
			if(taken == 0){ // no other thread wants the node
				nodes[found]->thread_id = thread_id;
				nodes[found]->line = line;
			}
		}
	}

	// add edges
	printf(">>> add edges\n");
	for(int i=0; i < NN+TN ; i++){ // for all nodes
		if(nodes[i] == 0x0 || i == found) {
			continue;
		}
		// existing node & not itself

		if(nodes[i]->thread_id == thread_id && i != found && nodes[i]->line != 0){ // with the same thread id
			//printf("nodes[%d]->thread_id = %lu\n", i, nodes[i]->thread_id);
			for(int j=0; j<NN; j++){
				if(nodes[i]->edges[j] == nodes[found]){ // already has an invalid edge
					// make that edge valid
					nodes[i]->threads[j] = thread_id;
					nodes[i]->valid[j] = 1;
					break;
				}
				if(nodes[i]->edges[j] == 0x0){ // add edge
					//printf("nodes[%d]->edges[%d]\n", i, j);
					nodes[i]->edges[j] = nodes[found];
					nodes[i]->threads[j] = thread_id;
					nodes[i]->valid[j] = 1;
					if(i >= NN){ // thread node일때
						nodes[i]->lines[j] = line;
					}
					break;
				}
			}
		}
	}
}

/*
   1. find node with the same mutex info
   2. delete edges from the same thread id nodes (from mutex & thread nodes)
   3. find whether there is a node that points to the node
   	3-1) if there is one, make the thread_id to unknown & delete edges in node
	3-2) if there is none, free all memory and delete node

 */
void unlock(node ** nodes, pthread_mutex_t * mutex, long thread_id, long line){
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

	// make all edges in the found node invalid
	for(int i=0; i<NN; i++){
		nodes[found]->valid[i] = 0;
	}

	// make all edges to the found node from the same thread id nodes invalid
	for(int i=0; i<NN+TN; i++){
		if(nodes[i] != 0x0 && nodes[i]->thread_id == thread_id){
			for(int j=0; j<NN; j++){
				if(nodes[i]->edges[j] == nodes[found]){
					nodes[i]->valid[j] = 0;
				}
			}
		}
	}

	int exist = -1;
	for(int i=NN; i< NN+TN; i++){
		if(nodes[i] != 0x0 && nodes[i]->thread_id != thread_id){ // other thread nodes
			for(int j=0; j<NN; j++){ // have valid edge pointing to the found node
				if(nodes[i]->valid[j] == 1 && nodes[i]->edges[j] != 0x0 && nodes[i]->edges[j] == nodes[found]){
					exist = i;
					break;
				}
			}
		}
	}
	if(exist != -1){
		//nodes[found]->thread_id = 0;
		nodes[found]->line = 0;
		//printf("nodes[found]->thread_id = %lu\n", nodes[found]->thread_id);
	}
}









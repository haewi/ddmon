#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>

#define NN 10 // maximum mutex number
#define TN 10 // maximum thread number

typedef enum {
	White, Gray, Black
} color;

typedef struct Node{
	pthread_mutex_t * mutex;
	long thread_id; // -1 means 'unknown'
	color col;
	struct Node ** edges;

} node;


int read_bytes(int fd, void * a, int len);
void init_node(node* n);
void print_graph(node ** nodes);

int main(){

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

	// init nodes (if mutex not held, it will be 0x0)
	node ** nodes = (node**) malloc((NN + TN)*sizeof(node*));
	for(int i = 0 ; i< NN+TN; i++){ // nodes for mutex
		nodes[i] = 0x0;
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

		printf("| %d - %ld - %p |\n", len, thread_id, mutex);

		// make thread nodes
		int none = -1;
		for(int i=NN; i< NN+TN; i++){
			if(nodes[i] != 0x0 && nodes[i]->thread_id == thread_id){ // thread node already exist
				printf("found = nodes[%d]->thread_id = %lu\n", i, nodes[i]->thread_id);
				none = i;
				break;
			}
		}
		if(none == -1){ // no thread node
			for(int i=NN; i < NN+TN; i++){ // for all thread nodes
				if(nodes[i] == 0x0) { // find empty thread node
					printf("found empty thread node: %d\n", i);
					nodes[i] = (node*) malloc(sizeof(node));
					init_node(nodes[i]);
					nodes[i]->thread_id = thread_id; // and fill it
					break;
				}
			}
		}
		printf("thread node done\n");

		print_graph(nodes);


		/*
		 RULE
		 the edges variable in the node structure points to the destination node (the edge is saved in the starting node)
		 */
		// handle info.
		if(len == 1) { // lock executed
			/*
			 	1. find node with the same mutex info
				   - found -> add edges = nodes with the same thread id -> found node
				   - not found -> add node & add edges = nodes with the same thread id -> made node
				2. adjust thread id of all nodes with the same thread id
			 */
			printf("> lock executed\n");

			int found=-1;
			for(int i=0; i<NN+TN; i++){ 
				if(nodes[i] != 0x0 && nodes[i]->mutex == mutex){ // if found node
					found=i; // found the node
					/*for(int j=0; j<NN; j++) { // for all nodes,
						if(nodes[j]->thread_id == thread_id && i != j) { // with the same thread id,
							for(int k=0; k<9; k++){
								if(nodes[j]->edges[k] == 0x0){ // add edge = same thread id node -> found node
									nodes[j]->edges[k] = nodes[i];
								}
								break;
							}
						}
					}*/
					break;
				}
			}

			if(found == -1){ // no node with the same mutex info.
				printf("\n>> no node\n");

				// make new node
				node *tmp = (node*) malloc(sizeof(node));
				printf("made node\n");
				init_node(tmp);
				tmp->mutex = mutex;
				tmp->thread_id = thread_id;

				// add node to nodes variable
				for(int i=0; i<NN; i++){
					if(nodes[i] == 0x0) { // a empty spot in the nodes variable
						printf("node[%d] = 0x0\n", i);
						nodes[i] = tmp;
						found = i;
						printf("added node on nodes[%d]\n", i);
						/*for(int j=0; j<NN; j++){ // for all nodes
							if(nodes[j]->thread_id == thread_id && i != j) { // with the same thread id
								for(int k=0; k<9; k++){
									if(nodes[j]->edges[k] == 0x0) { // add edge
									       nodes[j]->edges[k] = nodes[i];
									}
									break;
								}
							}
						}*/
						break;
					}
				}
			}
			else {
				printf("\n>> found node\n");
			}

			// add edges
			printf("\n>>> add edges\n");
			for(int i=0; i < NN+TN ; i++){ // for all nodes
				//printf("i=%d in for loop\n", i);
				if(nodes[i] == 0x0 || i == found) {
					continue;
				}
				// existing node & not itself

				if(nodes[i]->thread_id == thread_id && i != found){ // with the same thread id
					printf("nodes[%d]->thread_id = %lu\n", i, nodes[i]->thread_id);
					for(int j=0; j<NN; j++){
						if(nodes[i]->edges[j] == 0x0){ // add edge
							printf("nodes[%d]->edges[%d]\n", i, j);
							nodes[i]->edges[j] = nodes[found];
						}
						break;
					}
				}
			}

			
		} // lock executed
		else { // unlock executed
			printf("> unlock executed\n");
		}
		printf("done\n");

		print_graph(nodes);


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
	n->thread_id = -1;
	n->col = White;

	n->edges = (node**) malloc(9*sizeof(node*));
	for(int i=0; i<NN; i++){
		n->edges[i] = 0x0;
	}
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
	printf("|\n|mutex nodes info.\n|\n");
	for(int i=0; i<NN; i++){
		if(nodes[i] == 0x0) continue;
		printf("| - node[%d]=%p -> mutex: %p - id: %lu\n", i, nodes[i], nodes[i]->mutex, nodes[i]->thread_id);
		for(int j=0; j<NN; j++){
			if(nodes[i]->edges[j] != 0x0){
				printf("|   nodes[%d]->edges[%d] = %p\n", i, j, nodes[i]->edges[j]);
			}
		}
		printf("| \t --- edge printing done\n|\n");
	}
	printf("|thread nodes info. \n|\n");
	for(int i=NN; i< NN+TN; i++){
		if(nodes[i] == 0x0) continue;
		
		printf("in if statement\n");
		printf("| - node[%d]=%p -> id: %lu\n", i, nodes[i], nodes[i]->thread_id);
		for(int j=0; j<NN; j++){
			if(nodes[i]->edges[j] != 0x0){
				printf("|   nodes[%d]->edges[%d] = %p\n", i, j, nodes[i]->edges[j]);
			}
		}
		printf("| \t --- edge printing done\n|\n");
	}
	
	printf("|-----------------------------------------------------------------------|\n\n");
}



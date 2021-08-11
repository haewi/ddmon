#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>

#define NN 10 // maximum node number

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
	printf("ddchck - MADE channel\n");

	int ddtrace = open(".ddtrace", O_RDWR | O_SYNC);
	printf("ddchck - opened channel\n");
	if(ddtrace < 0){
		fputs("[Error] ddchck - can't open .ddtrace\n", stderr);
	}

	// init nodes (if mutex not held, it will be 0x0)
	node ** nodes = (node**) malloc(10*sizeof(node*));
	for(int i =0 ; i< NN; i++){
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

		printf("< %d - %ld - %p\n >", len, thread_id, mutex);

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
			printf("lock executed\n");

			int found=-1;
			for(int i=0; i<NN; i++){ 
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
				printf("no node\n");

				// make new node
				node tmp;
				init_node(&tmp);
				tmp.mutex = mutex;
				tmp.thread_id = thread_id;
				printf("made node\n");

				// add node to nodes variable
				for(int i=0; i<NN; i++){
					printf("in for loop\n");
					if(nodes[i] == 0x0) { // a empty spot in the nodes variable
						printf("node[%d] = 0x0\n", i);
						nodes[i] = &tmp;
						found = i;
						printf("added node\n");
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
				printf(" found node\n");
			}

			// add edges
			printf("\nbefore adding edges\n");
			for(int i=0; i<NN; i++){ // for all nodes
				printf("in for loop\n");
				if(nodes[i] == 0x0 || i == found) continue;
				printf("nodes[%d] not Null\n", i);
				if(nodes[i]->thread_id == thread_id && i != found){ // with the same thread id
					printf("nodes[%d]->thread_id = %lu\n", i, nodes[i]->thread_id);
					for(int j=0; j<9; j++){
						if(nodes[i]->edges[j] == 0x0){ // add edge
							printf("nodes[%d]->edges[%d]", i, j);
							nodes[i]->edges[j] = nodes[i];
						}
						break;
					}
				}
			}

			
		} // lock executed
		else { // unlock executed
			printf("unlock executed\n");
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
	for(int i=0; i<9; i++){
		n->edges[i] = 0x0;
	}
}

void print_graph(node ** nodes) {
	printf("nodes' address\n");
	for(int i=0; i<NN; i++){
		if(nodes[i] != 0x0){
			printf("%p ", nodes[i]);
		}
	}
	printf("\n\n\tnode info.\n");
	for(int i=0; i<NN; i++){
		printf("----- node= mutex: %p - id: %lu -----\n", nodes[i]->mutex, nodes[i]->thread_id);
		for(int j=0; j<9; j++){
			if(nodes[i]->edges[j] != 0x0){
				printf("%p\n", nodes[i]->edges[j]);
			}
		}
	}
}



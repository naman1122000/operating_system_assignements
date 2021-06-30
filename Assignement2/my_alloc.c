#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#define FREE_LIST_OFFSET 32
#define MAGIC_NUMBER_CONST 1234567 
#define NODE_SIZE 16

char* start_pointer = NULL;

// free list node;
struct free_list_node{
	int size;
	struct free_list_node* next;
};
// malloc header
struct malloc_header{
	int size;
	int magic_number;
};


// Implement these yourself
int my_init(){
	start_pointer = mmap(NULL, 4096, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANON|MAP_PRIVATE, 0, 0); 
	if(start_pointer == MAP_FAILED || start_pointer == NULL) {
    	return errno;
  	}
  	struct free_list_node head;
  	
  	char* x = start_pointer + FREE_LIST_OFFSET;
  	head.size = 4096 - (FREE_LIST_OFFSET + NODE_SIZE);
  	head.next = NULL;

  	memcpy(start_pointer, &x, 8); // storing the address of head of the free_list
  	memcpy(x, &head, sizeof(head)); // storing the head of free_list
  	
  	int y = 4096 - 24 - 8;
  	memcpy((start_pointer + 8), &y, 4);// max size
  	
  	y = 16; // memory initially used to maintain free list;
  	memcpy((start_pointer + 12), &y, 4);// current size

  	y = 4096 - 24 - 8 - 16;
  	memcpy((start_pointer + 16), &y, 4);// free memory

  	y = 0; 
  	memcpy((start_pointer + 20), &y, 4); // // number of blocks allocated 	

  	y = 4096 - 24 - 8 - 16;
  	memcpy((start_pointer + 24), &y, 4); // size of smallest available chunk

  	y = 4096 - 24 - 8 - 16;
  	memcpy((start_pointer + 28), &y, 4); // size of largest available chunk	
	return 0;
}
void increaseAllocatedBlocks(){
	int nAllocatedBlocks = 0;
	memcpy(&nAllocatedBlocks, start_pointer+20, 4);
	nAllocatedBlocks++; 
	memcpy((start_pointer+20), &nAllocatedBlocks, 4); // // number of blocks allocated	
	return;
}
void decreaseAllocatedBlocks(){
	int nAllocatedBlocks = 0;
	memcpy(&nAllocatedBlocks, start_pointer+20, 4);
	nAllocatedBlocks--; 
	memcpy((start_pointer+20), &nAllocatedBlocks, 4); // // number of blocks allocated	
	return;
}
void increaseCurrentSize(int size){
	int s;
	memcpy(&s, start_pointer + 12, 4);
	s += size;
	// printf("%d\n", s);
	memcpy(start_pointer + 12, &s, 4);
	return; 
}

void updateFreeMemory(int size){
	int s;
	memcpy(&s, start_pointer + 16, 4);
	s += size;
	// printf("%d\n", s);
	memcpy(start_pointer + 16, &s, 4);
	return; 	
}

int getSmallestChunkSize(){
	int s;
	memcpy(&s, start_pointer + 24, 4);
	return s;
}

int getLargestChunkSize(){
	int s;
	memcpy(&s, start_pointer + 28, 4);
	return s;
}

void updateLargestChunk(int size){
	if(size == -1){
		struct free_list_node* head;
		memcpy(&head, start_pointer, 8);
		size = 0;
		while(head != NULL){
			if(head->size > size){
				size = head->size;
			}
			head = head->next;
		}
	}
	memcpy(start_pointer + 28, &size, 4);
}

void updateSmallestChunk(int size){
	if(size == -1){
		struct free_list_node* head;
		memcpy(&head, start_pointer, 8);
		size = 4096;
		if(head == NULL){
			size = 0;
		}
		while(head != NULL){
			if(head->size < size && size != 0){
				size = head->size;
			}
			head = head->next;
		}
		if(size == 4096){
			size = 0;
		}
	}
	memcpy(start_pointer + 24, &size, 4);
}

void* my_alloc(int count){

	if(count%8 != 0 || count < 8 || start_pointer == NULL){
		return NULL;
	}

	struct free_list_node* head = NULL;
	memcpy(&head, start_pointer, sizeof(head)); // computing head of free list 
	
	if(head == NULL){
		return NULL;
	}

	void* return_address = NULL;
	if((head->size) + 8 >= count){
 		if((head->size) + 8 - count < 16){
 			// if any node size that can be allocated - size asked < 16
 			// then we will allocate all of this free node to the user 
 			// because remaining < 16 cannot store the free_list_node
 			int initialSize = head->size;

 			updateFreeMemory(-(head->size));
 			struct free_list_node* next_node;
        	struct malloc_header temp;
        	temp.size = (head->size) + 8;
        	temp.magic_number = MAGIC_NUMBER_CONST;

        	next_node = head->next;
        	memcpy(head, &temp, 8);
        	return_address = ((void*)head) + 8;

        	head = next_node;
        	char* temp2 = (char*)head;
            memcpy(start_pointer, &temp2, 8);
 			if((initialSize) == getSmallestChunkSize()){
 				updateSmallestChunk(-1);
 			}

 			if((initialSize) == getLargestChunkSize()){
 				updateLargestChunk(-1);
 			}
 			increaseCurrentSize(initialSize);

 		}
 		else{
 			if(((head->size) - (count - 8) - 16) < getSmallestChunkSize() && ((head->size) - (count - 8) - 16) != 0){
 				updateSmallestChunk(((head->size) - (count - 8) - 16));
 			}
 			int initialSize = head->size; 
 
 			updateFreeMemory(-(count+8));
 			// split the node first part will be returned second will remain in free list
        	struct malloc_header temp;
        	struct free_list_node tempf;
        	return_address = (void*)head + 8;
        	tempf = *head;
        	tempf.size = (head->size) - (count + 8);

        	temp.size = count;

        	temp.magic_number = MAGIC_NUMBER_CONST;

			memcpy(head, &temp, 8);

        	char* temp2 = (char*)head;			
			temp2 = temp2 + count + 8;
			memcpy(temp2, &tempf, 16);
			memcpy(start_pointer, &temp2, 8);
			increaseCurrentSize(count - 8 + 16);
			if(initialSize == getLargestChunkSize()){
				updateLargestChunk(-1);
			}
			if(initialSize == getSmallestChunkSize()){
				updateSmallestChunk(-1);	
			}

 		}
 		increaseAllocatedBlocks();
        return (return_address);
	}

    struct free_list_node* tempp = head;
	struct free_list_node* temp = head->next;

	while(temp != NULL){
		if((temp->size) + 8 >= count){
            if((temp->size) + 8 - count < 16){
				// if any node size that can be allocated - size asked < 16
	 			// then we will allocate all of this free node to the user 
	 			// because remaining < 16 cannot store the free_list_node
	 			int initialSize = temp->size;

	 			updateFreeMemory(-(temp->size));
            	struct free_list_node* next_node;
	        	struct malloc_header temp1;
	        	temp1.size = temp->size + 8;
	        	temp1.magic_number = MAGIC_NUMBER_CONST;
	        	next_node = temp->next;
	        	memcpy(temp, &temp1, 8);
	        	return_address = (void*)temp + 8;

	        	tempp->next = next_node; 
	 			if(initialSize == getSmallestChunkSize()){
 					updateSmallestChunk(-1);
 				}
	 			if(initialSize == getLargestChunkSize()){
 					updateLargestChunk(-1);
 				}
 				increaseCurrentSize(initialSize);
            }
            else{
	 			// split the node first part will be returned second will remain in free list
	        	if((temp->size) - (count - 8) - 16 < getSmallestChunkSize() &&  ((temp->size) - (count - 8) - 16) != 0){
 					updateSmallestChunk((temp->size) - (count - 8) - 16);
 				}
 				int initialSize = temp->size; 
	        	updateFreeMemory(-(count + 8));
	        	struct malloc_header mh_temp;
	        	struct free_list_node tempf;
	        	tempf = *temp;
	        	tempf.size = (temp->size) - (count + 8);

	        	mh_temp.size = count ;
	        	mh_temp.magic_number = MAGIC_NUMBER_CONST;
				memcpy(temp, &mh_temp, 8);

				return_address = (void*)temp + 8;
				
				char* temp2 = (char*)temp;			
				temp2 = temp2 + count + 8;
				memcpy(temp2, &tempf, 16);
				memcpy(&(tempp->next), &temp2, 8);
				increaseCurrentSize(count - 8 + 16);
				if(initialSize == getLargestChunkSize()){
					updateLargestChunk(-1);
				}
				if(initialSize == getSmallestChunkSize()){
					updateSmallestChunk(-1);	
				}
            }
            increaseAllocatedBlocks();
        	return (return_address);
		}

		temp = temp->next;
		tempp = tempp->next;
	}

	return NULL; // no memory available

}

void my_free(void *ptr){
	if(ptr == NULL || start_pointer == NULL) return;
	// Replace free with your functionality
    struct malloc_header* p = (struct malloc_header*)ptr - 1;
    // printf("here8\n");
    //printf("here");
    if(p->magic_number != MAGIC_NUMBER_CONST){
    	return;
    }
   // printf("here2");
    p->magic_number = 0;// changing magic number
	struct free_list_node* head = NULL;
	memcpy(&head, start_pointer, 8); // computing head of free list 
  	// printf("here6\n");
	// printf("here7\n");
	struct free_list_node* itr = NULL;
	struct free_list_node* itrp = NULL;
	struct free_list_node* afterp = NULL;
	itr = head;

 	struct free_list_node* before = NULL;
    struct free_list_node* after = NULL;
	while(itr != NULL){
		char* t = (char*)itr;
		if(t + (itr->size) + 16 == (char*)p){
			before = itr;
		}

		t = (char*)p; 
		if(t + (p->size) + 8 == (char*)itr){
			after = itr;
			afterp = itrp;
			
		} 
		itrp = itr;
		itr = itr->next;
	} 
	if(before != NULL && after != NULL){
		char flag = '0';
		if((before->size) + (after->size) + (p->size) + 8 + 16 > getLargestChunkSize()){
			updateLargestChunk((before->size) + (after->size) + (p->size) + 8 + 16);
		}
		if((before->size) + (after->size) + (p->size) + 8 + 16 < getSmallestChunkSize()){
			updateSmallestChunk((before->size) + (after->size) + (p->size) + 8 + 16);
		}

       	if(before->size == getSmallestChunkSize() || after->size == getSmallestChunkSize()){
       		flag = '1';
       	}
		// printf("here1\n");
		increaseCurrentSize(-((p->size) + 8 + 16));
		updateFreeMemory(p->size + 16 + 8);
		if((before->size))
		before->size += (p->size) + 8 + (after->size) + 16;

		if(afterp == NULL){
			memcpy(start_pointer, &(after->next), 8);
		}
		else{
			afterp->next = after->next;
		}
		decreaseAllocatedBlocks();
		if(flag == '1'){
			updateSmallestChunk(-1);
		}
		return;
	}
	if(before == NULL && after != NULL){
		//printf("here2\n");
		char flag = '0';
		if((after->size) + (p->size) + 8  > getLargestChunkSize()){
			updateLargestChunk((after->size) + (p->size) + 8);
		}
       	if(after->size == getSmallestChunkSize()){
       		flag = '1';
       	}
		if((after->size) + (p->size) + 8 < getSmallestChunkSize()){
			updateSmallestChunk((after->size) + (p->size) + 8);
		}
		increaseCurrentSize(-((p->size) + 8));
		updateFreeMemory(p->size + 8);
		//printf("here4\n");
		char* t = (char*) p;
		struct free_list_node f1;
		f1.size = (p->size) - 8 + ((after->size) + 16);
		f1.next = after->next;
		memcpy(t, &f1, 16);
		//printf("here5\n");
		if(afterp == NULL){
			memcpy(start_pointer, &t, 8);
		}
		else{
			//printf("here8\n");
			memcpy(&(afterp->next), &t, 8);	
		}
		//printf("here7\n");
		decreaseAllocatedBlocks();
		//printf("here6\n");
		if(flag == '1'){
			updateSmallestChunk(-1);
		}
		//printf("here3\n");
		return;
	}
	if(before != NULL && after == NULL){
		// printf("here3\n");
		char flag = '0';
		if((before->size) + (p->size) + 8  > getLargestChunkSize()){
			updateLargestChunk((before->size) + (p->size) + 8);
		}
		if((before->size) + (p->size) + 8 < getSmallestChunkSize()){
			updateSmallestChunk((before->size) + (p->size) + 8);
		}
       	if(before->size == getSmallestChunkSize()){
       		flag = '1';
       	}
		increaseCurrentSize(-((p->size) + 8));
		updateFreeMemory(p->size + 8);
		before->size = (before->size) + (p->size) + 8;
		decreaseAllocatedBlocks();
		if(flag == '1'){
			updateSmallestChunk(-1);
		}
		return;
	}
	if(before == NULL && after == NULL){
		//printf("here");
		if(head == NULL){
			updateLargestChunk((p->size) + 8 - 16);
			updateSmallestChunk((p->size) + 8 - 16);
		}

		if((p->size) + 8 - 16 > getLargestChunkSize()){
			updateLargestChunk((p->size) + 8 - 16);
		}

		if((p->size) + 8 - 16 < getSmallestChunkSize() && (p->size) + 8 - 16 != 0){
			updateSmallestChunk((p->size) + 8 - 16);
		}

		// printf("here4\n");
		increaseCurrentSize(-((p->size) - 8));
		updateFreeMemory(p->size - 8);
		char* t = (char*)p;

		struct free_list_node f1;
		f1.size = (p->size) - 8;
		f1.next = head;
		memcpy(t, &f1, 16); // overwriting malloc node with free list node

		memcpy(start_pointer, &t, 8); // updating head of free list
		decreaseAllocatedBlocks();
		return;
	}
	// printf("here5\n");
	return;
}

void my_clean(){
	if(start_pointer != NULL &&  start_pointer != MAP_FAILED){
	  	int y = 0;
	  	memcpy((start_pointer + 8), &y, 4);// max size
	  	
	  	y = 0; // memory initially used to maintain free list;
	  	memcpy((start_pointer + 12), &y, 4);// current size

	  	y = 0;
	  	memcpy((start_pointer + 16), &y, 4);// free memory

	  	y = 0; 
	  	memcpy((start_pointer + 20), &y, 4); // // number of blocks allocated 	

	  	y = 0;
	  	memcpy((start_pointer + 24), &y, 4); // size of smallest available chunk

	  	y = 0;
	  	memcpy((start_pointer + 28), &y, 4); // size of largest available chunk	

	    munmap(start_pointer, 4096);
	}
	return;
}

void my_heapinfo(){
	
	int a, b, c, d, e, f;
	
	memcpy(&a, start_pointer + 8, 4);
	memcpy(&b, start_pointer + 12, 4);
	memcpy(&c, start_pointer + 16, 4);
	memcpy(&d, start_pointer + 20, 4);
	memcpy(&e, start_pointer + 24, 4);
	memcpy(&f, start_pointer + 28, 4);

	// Do not edit below output format
	printf("=== Heap Info ================\n");
	printf("Max Size: %d\n", a);
	printf("Current Size: %d\n", b);
	printf("Free Memory: %d\n", c);
	printf("Blocks allocated: %d\n", d);
	printf("Smallest available chunk: %d\n", e);
	printf("Largest available chunk: %d\n", f);
	printf("==============================\n");
	// Do not edit above output format

	return;
}
void print_free_list(){
	struct free_list_node* c;
	memcpy(&c, start_pointer, 8);
	while(c != NULL){
		printf("c %d\n", (c->size));
		c = c->next;
	}
}
// int main(int argc, char *argv[]){
// 	my_init();
// 	printf("%p", start_pointer);
// 	// printf("%p\n", *start_pointer);
// 	my_heapinfo();
// 	void* a1 = my_alloc(16);
// 	my_heapinfo();
// 	void* a2 = my_alloc(8);
// 	my_heapinfo();
// 	void* a3 = my_alloc(24);
// 	my_heapinfo();

// 	struct free_list_node* c;
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("c %d\n", (c->size));
// 		c = c->next;
// 	}
// 	my_free(NULL);
// 	my_free(a1);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc %d\n", (c->size));
// 		c = c->next;
// 	}


// 	my_free(a3);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc2 %d\n", (c->size));
// 		c = c->next;
// 	}

// 	my_alloc(32);
// 	my_heapinfo();

// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc3 %d\n", (c->size));
// 		c = c->next;
// 	}

// 	my_free(a2);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc3 %d\n", (c->size));
// 		c = c->next;
// 	}
// 	printf("---------------------------------\n");
// 	char* c1 = my_alloc(24);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc4 %d\n", (c->size));
// 		c = c->next;
// 	}
// 	char* c2 = my_alloc(32);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc4 %d\n", (c->size));
// 		c = c->next;
// 	}
// 	my_free(c1);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc5 %d\n", (c->size));
// 		c = c->next;
// 	}
// 	my_free(c2);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc4 %d\n", (c->size));
// 		c = c->next;
// 	}
// 	char* c3 = my_alloc(40);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc5 %d\n", (c->size));
// 		c = c->next;
// 	}

// 	char* c4 = my_alloc(40);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc6 %d\n", (c->size));
// 		c = c->next;
// 	}

// 	my_free(c3);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc7 %d\n", (c->size));
// 		c = c->next;
// 	}
// 	char* c5 = my_alloc(16);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc7 %d\n", (c->size));
// 		c = c->next;
// 	}
// 	char* c6 = my_alloc(24);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc7 %d\n", (c->size));
// 		c = c->next;
// 	}
// 	my_free(c6);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc7 %d\n", (c->size));
// 		c = c->next;
// 	}
// 	my_free(c6);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc7 %d\n", (c->size));
// 		c = c->next;
// 	}
// 	my_alloc(16);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc7 %d\n", (c->size));
// 		c = c->next;
// 	}
// 	char* z = my_alloc(3872-8);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc7 %d\n", (c->size));
// 		c = c->next;
// 	}
// 	my_free(z);
// 	my_heapinfo();
// 	memcpy(&c, start_pointer, 8);
// 	while(c != NULL){
// 		printf("sc7 %d\n", (c->size));
// 		c = c->next;
// 	}
// 	my_clean();
// 	printf("%p", start_pointer);
// }
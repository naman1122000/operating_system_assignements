#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define TOTAL_PAGE 1048576
#define PFN_MASK 0xFFF00000
#define PFN_SHIFT 20
#define VALID_MASK 0x00080000
#define DIRTY_MASK 0x00040000
#define PAGE_MASK 0xFFFFF000
#define PAGE_SHIFT 12

int page_table[TOTAL_PAGE] = {0};
int n = 0;
int v = 0;
char* trace_file;
int* address;
int count; 
char* access;
int tme = 0; // number of memory access
int misses = 0; // total misses
int writes = 0; // total writes
int drops = 0; // total drops
struct LRU_struct{
	int page_number;
	int time;
};
char* cut_temp(char* temp){
    while(isspace(*temp)){
        temp++;
    }
    if(strlen(temp) == 0) return temp;
    char* last = temp + strlen(temp);
    if(*(last - 1) == '\n') last--;
    while(isspace(*--last));
    *(last+1) = '\0';
    return temp;
}
int count_line(char* trace_file){
	count = 0;

    FILE * fp;
    fp = fopen(trace_file, "r");
    if(!fp) exit(1);

    while(!feof(fp)){
        char temp[30];
    	fgets(temp, sizeof(temp), fp);
        char* temp1 = cut_temp(temp);
        if(strlen(temp1) < 6){
            break;
        }
        //printf("%s\n", temp1);
    	temp[0] = '\0';
    	count++;
    }
    fclose(fp);
}

int getPageNo(int address){
	return (address & PAGE_MASK) >> PAGE_SHIFT;	
}

void removePageFromFrame(int* frame, int frame_number){
	int page_number = frame[frame_number];
	if(page_table[page_number] & DIRTY_MASK){
        if(v == 1)
            printf(" page 0x%05x was written to the disk.\n", page_number);
		writes++;
	}
	else{
        if(v == 1)
            printf(" page 0x%05x was dropped (it was not dirty).\n", page_number);
		drops++;
	}
	page_table[page_number] = 0; 
	frame[frame_number] = -1;
}

void removePageFromFrameForLru(struct LRU_struct* frame, int frame_number){
	int page_number = frame[frame_number].page_number;
	if(page_table[page_number] & DIRTY_MASK){
        if(v == 1)
            printf(" page 0x%05x was written to the disk.\n", page_number);
		writes++;
	}
	else{
        if(v == 1)
            printf(" page 0x%05x was dropped (it was not dirty).\n", page_number);
		drops++;
	}
	page_table[page_number] = 0; 
	frame[frame_number].page_number = -1;
	frame[frame_number].time = -1;
}
void printOutput(){
		printf("Number of memory accesses: %d\n", count);
		printf("Number of misses: %d\n", misses);
		printf("Number of writes: %d\n", writes);
        printf("Number of drops: %d\n", drops);
}
void implementOPT(){
 
    int frame[n];
   	for(int i = 0; i < n; i++){
   		frame[i] = -1;
   	}

    int total_frame = 0; // total pages in frame initially
    for(int i = 0; i < count; i++){
    	int page_number = getPageNo(address[i]);
    	//printf("page_number: %d\n", page_number);
    	int pte = page_table[page_number];
    	if(pte & VALID_MASK){
    		// it is in the frame already, update dirty bit
    		if(access[i] == 'W'){
    			page_table[page_number] = page_table[page_number] | DIRTY_MASK;
    		}
    	}
    	else{
    		//printf("page_number: %d\n", page_number);
    		misses++;
    		// valid bit is 0
    		int frame_number = -1;
    		// it is not present in frame 
    		if(total_frame < n){
    			// Find the empty frame
    			for(int j = 0; j < n; j++){
    				if(frame[j] == -1){
    					frame_number = j;
						break;
    				}
    			}
    			total_frame++;
    		}
    		else{
                if(v == 1)
                  printf("Page 0x%05x was read from disk,", page_number);
    			int max_future_index = -1;
    			for(int j = 0; j < n; j++){
    				int k = i+1;
    				for(; k < count; k++){
    					if(getPageNo(address[k]) == frame[j]){
							break;
     					}
    				}
    				if(k > max_future_index){
    					frame_number = j;
    					max_future_index = k;
    				}

    				if(max_future_index == count) 
    					break;
    			}
    			removePageFromFrame(frame, frame_number);
    			// printf("i: %d\n", i);
    			// for(int i = 0; i < n; i++){
    			// 	printf("%d ", frame[i]);
    			// } 
    			// printf("\n");
    			//printf("%05x",a); 
    		}
    		// upadte page frame number
    		// modify valid bit
    		// modify dirty bit
    		frame[frame_number] = page_number;
    		page_table[page_number] = frame_number << PFN_SHIFT;
    		page_table[page_number] = page_table[page_number] | VALID_MASK;
    		if(access[i] == 'W'){
    			page_table[page_number] = page_table[page_number] | DIRTY_MASK;
    		} 
    		//printOutput();
    	}

	}
	printOutput();
}

void implementFIFO(){
 
    int frame[n];
   	for(int i = 0; i < n; i++){
   		frame[i] = -1;
   	}

    int total_frame = 0; // total pages in frame initially
    int s = 0;
    int e = 0;
    for(int i = 0; i < count; i++){
    	int page_number = getPageNo(address[i]);
    	// printf("page_number: %d\n", page_number);
    	int pte = page_table[page_number];
    	if(pte & VALID_MASK){
    		// it is in the frame already, update dirty bit
    		if(access[i] == 'W'){
    			page_table[page_number] = page_table[page_number] | DIRTY_MASK;
    		}
    	}
    	else{
    		misses++;
    		// valid bit is 0
    		int frame_number = -1;
    		// it is not present in frame 
    		if(total_frame < n){
    			total_frame++;
    			frame_number = e;
    			e++;
    			e = e%n;
    		}
    		else{
                if(v == 1)
                  printf("Page 0x%05x was read from disk,", page_number);
    			frame_number = e;
    			e++;
    			e %= n;
    			s++;
    			s %= n;
    			removePageFromFrame(frame, frame_number);
    			// printf("i: %d\n", i);
    			// for(int i = 0; i < n; i++){
    			// 	printf("%d ", frame[i]);
    			// } 
    			// printf("\n");
    			//printf("%05x",a); 
    		}
    		// upadte page frame number
    		// modify valid bit
    		// modify dirty bit
    		frame[frame_number] = page_number;
    		page_table[page_number] = frame_number << PFN_SHIFT;
    		page_table[page_number] = page_table[page_number] | VALID_MASK;
    		if(access[i] == 'W'){
    			page_table[page_number] = page_table[page_number] | DIRTY_MASK;
    		} 
    		//printOutput();
    	}

	}
	printOutput();

}

void implementCLOCK(){

    struct LRU_struct frame[n];
   	for(int i = 0; i < n; i++){
   		frame[i].time = 0; 
   		frame[i].page_number = -1;
   	}

   	int s = 0;
    int total_frame = 0; // total pages in frame initially
    for(int i = 0; i < count; i++){
    	int page_number = getPageNo(address[i]);
    	// printf("page_number: %d\n", page_number);
    	int pte = page_table[page_number];
    	if(pte & VALID_MASK){
    		int PFN = pte & PFN_MASK;
    		PFN = PFN >> PFN_SHIFT;
    		frame[PFN].time = 1;
    		// it is in the frame already, update dirty bit
    		if(access[i] == 'W'){
    			page_table[page_number] = page_table[page_number] | DIRTY_MASK;
    		}
    	}
    	else{
    		misses++;
    		// valid bit is 0
    		int frame_number = -1;
    		// it is not present in frame 
    		if(total_frame < n){
    			frame_number = total_frame;
    			total_frame++;
    		}
    		else{
                if(v == 1)
                  printf("Page 0x%05x was read from disk,", page_number);
    			while(frame[s].time != 0){
    				frame[s].time = 0;
    				s++;
    				s %= n;
	    		}
	    		frame_number = s;
	    		s++; s %= n;
    			removePageFromFrameForLru(frame, frame_number);
    			// printf("i: %d\n", i);
    			// for(int i = 0; i < n; i++){
    			// 	printf("%d ", frame[i].page_number);
    			// } 
    			// printf("\n");
    			//printf("%05x",a); 
    		}
    		// upadte page frame number
    		// modify valid bit
    		// modify dirty bit
    		frame[frame_number].page_number = page_number;
    		frame[frame_number].time = 1;
    		page_table[page_number] = frame_number << PFN_SHIFT;
    		page_table[page_number] = page_table[page_number] | VALID_MASK;
    		if(access[i] == 'W'){
    			page_table[page_number] = page_table[page_number] | DIRTY_MASK;
    		} 
    		//printOutput();
    	}

	}
	printOutput();	

}

void implementLRU(){

    struct LRU_struct frame[n];
   	for(int i = 0; i < n; i++){
   		frame[i].time = -1; 
   		frame[i].page_number = -1;
   	}

    int total_frame = 0; // total pages in frame initially
    for(int i = 0; i < count; i++){
    	int page_number = getPageNo(address[i]);
    	// printf("page_number: %d\n", page_number);
    	int pte = page_table[page_number];
    	if(pte & VALID_MASK){
    		int PFN = pte & PFN_MASK;
    		PFN = PFN >> PFN_SHIFT;
    		frame[PFN].time = i;
    		// it is in the frame already, update dirty bit
    		if(access[i] == 'W'){
    			page_table[page_number] = page_table[page_number] | DIRTY_MASK;
    		}
    	}
    	else{
    		misses++;
    		// valid bit is 0
    		int frame_number = -1;
    		// it is not present in frame 
    		if(total_frame < n){
    			frame_number = total_frame;
    			total_frame++;
    		}
    		else{
                if(v == 1)
                  printf("Page 0x%05x was read from disk,", page_number);
    			int min_time = count;
    			for(int j = 0; j < n; j++){
    				if(frame[j].time < min_time){
    					min_time = frame[j].time;
    					frame_number = j;
    				}
    			}
    			removePageFromFrameForLru(frame, frame_number);
    			// printf("i: %d\n", i);
    			// for(int j = 0; j < n; j++){
    			// 	printf("%d ", frame[j].page_number);
    			// } 
    			// printf("\n");
    			//printf("%05x",a); 
    		}
    		// upadte page frame number
    		// modify valid bit
    		// modify dirty bit
    		frame[frame_number].page_number = page_number;
    		frame[frame_number].time = i;
    		page_table[page_number] = frame_number << PFN_SHIFT;
    		page_table[page_number] = page_table[page_number] | VALID_MASK;
    		if(access[i] == 'W'){
    			page_table[page_number] = page_table[page_number] | DIRTY_MASK;
    		} 
    		//printOutput();
    	}

	}
	printOutput();	
}

void implementRANDOM(){
    srand(5635);
    int frame[n];
   	for(int i = 0; i < n; i++){
   		frame[i] = -1;
   	}

    int total_frame = 0; // total pages in frame initially
    for(int i = 0; i < count; i++){
    	int page_number = getPageNo(address[i]);
    	// printf("page_number: %d\n", page_number);
    	int pte = page_table[page_number];
    	if(pte & VALID_MASK){
    		// it is in the frame already, update dirty bit
    		if(access[i] == 'W'){
    			page_table[page_number] = page_table[page_number] | DIRTY_MASK;
    		}
    	}
    	else{
    		misses++;
    		// valid bit is 0
    		int frame_number = -1;
    		// it is not present in frame 
    		if(total_frame < n){
    			frame_number = total_frame;
    			total_frame++;
    		}
    		else{
                if(v == 1)
                  printf("Page 0x%05x was read from disk,", page_number);
    			frame_number = rand()%n;
    			removePageFromFrame(frame, frame_number);
    			// printf("i: %d\n", i);
    			// for(int j = 0; j < n; j++){
    			// 	printf("%d ", frame[j]);
    			// } 
    			// printf("\n");
    			//printf("%05x",a); 
    		}
    		// upadte page frame number
    		// modify valid bit
    		// modify dirty bit
    		frame[frame_number] = page_number;
    		page_table[page_number] = frame_number << PFN_SHIFT;
    		page_table[page_number] = page_table[page_number] | VALID_MASK;
    		if(access[i] == 'W'){
    			page_table[page_number] = page_table[page_number] | DIRTY_MASK;
    		} 
    		//printOutput();
    	}

	}
	printOutput();
}

int main(int argc, char *argv[]){
	trace_file = argv[1];
    n = atoi(argv[2]);
    char* strategy = argv[3];
    v = 0;
    if(argc == 5 && strcmp(argv[4], "-verbose") == 0){
    	v = 1;
    }

	count_line(trace_file);

	address = (int*)malloc(count*sizeof(int));
	access = (char*)malloc(count*sizeof(char));;
	FILE * fp;
	//printf("%d", count);
    fp = fopen(trace_file, "r");
    if (fp == NULL)
    	exit(EXIT_FAILURE);

    char temp[30];
    int i = 0;
   	for(int i = 0; i < count; i++){
    	fscanf(fp, "%s", temp);
    	if(strlen(temp) == 0){
    		break;
    	}
    	address[i] = strtol(temp, NULL, 16);
    	fscanf(fp, "%s", temp);
    	access[i] = temp[0];
    	temp[0] = '\0';
        
    }

    if(strcmp(strategy, "OPT") == 0){
    	implementOPT();
    }
    else if(strcmp(strategy, "FIFO") == 0){
    	implementFIFO();
    }
    else if(strcmp(strategy, "CLOCK") == 0){
    	implementCLOCK();
    }
    else if(strcmp(strategy, "LRU") == 0){
    	implementLRU();
    }
    else if(strcmp(strategy, "RANDOM") == 0){
    	implementRANDOM();
    }
    fclose(fp);
}
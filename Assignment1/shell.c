#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#define CMD_MAX 130 // max length of command
#define MAX_ARG 128  // max argument length + 1 of any command

char* history[5];
int hl; // number of commands executed
char codeD[PATH_MAX]; // stores the location of ~

// Find the current directory of the process	
void findCurrentDirectory(char currD[PATH_MAX]){
   if (getcwd(currD, PATH_MAX) == NULL) {
    	perror("Error: Not able to find the current directory getcwd failed");
   } 
}

// saves commands by spliting in the array and returns length of split array
int decodeCommand(char* cmd, char* command[128]){
	int i = 0;
	int len = strlen(cmd);
	int j = 0;
	int k = 0;
	int dq = 0;
	command[0] = (char*)calloc(129, sizeof(char));
	while(i < len){
		if((cmd[i] == '\"' || cmd[i] == '\'') && dq == 0){
			dq = 1;
		}
		else if((cmd[i] == '\"' || cmd[i] == '\'') && dq == 1){
		   		dq = 0;
		}
		else if(cmd[i] == ' ' && dq == 1){
			command[j][k] = cmd[i];
			k++;			
		}
		else if(cmd[i] == ' ' && dq == 0){
			i++;
			while(i < len && cmd[i] == ' '){
				i++;
			}
			command[j][k] = '\0';
			j++;
			command[j] = (char*)calloc(129, sizeof(char));
			k = 0;
			continue;
		}
		else if(cmd[i] == '~' && dq == 0){
			for(int x = 0;x < strlen(codeD); x++){
				command[j][k] = codeD[x];
				k++;
			}
		}
		else{
			command[j][k] = cmd[i]; 	
			k++;		
		}
		i++;
	}
	if(dq == 1){
		return -1;
	}
    command[j][k] = '\0';
    return (j+1);
}

// map address so that it can be passed in chdir
void mapaddress(char* originalAddress, char* transformedAddress){
	if(originalAddress[0] == '~' && (strlen(originalAddress) == 1 || originalAddress[1] == '/')){
	  strcpy(transformedAddress, codeD);
	  strcat(transformedAddress, ((char*)(originalAddress+1)));
	}
	else{
		strcpy(transformedAddress, originalAddress);
	}
	return;
}

// mapped address so that it can be printed to terminal
void mapToPrintAddress(char currD[PATH_MAX], char* address){
	int flag = 0;
	if(strlen(currD) < strlen(codeD)){
		flag = 1;
	}
	else{
		for(int i = 0; i < strlen(codeD) && i < strlen(currD); i++){
			if(codeD[i] != currD[i]){
				flag = 1;
				break;
			}
		}
	}
	if(flag == 0){ 
		strcpy(address, "~");
		strcat(address, ((char*)(currD + (strlen(codeD)))));
	}
	else{
		strcpy(address, currD);
	}
}

//execute the cd command return 0 if succesfull else return -1;
int executeCd(char* command[MAX_ARG], char currD[PATH_MAX], int splitCommandLength){
	int flag = 0;
	if(splitCommandLength > 2){	
		fprintf(stderr, "Invalid number of arguments\n");
		return -1;
	}

	if(splitCommandLength == 1){
		flag = chdir(codeD);
		return 0;
	}
	else{
		if(command[1] == ".."){
			flag = chdir(".."); 
		}
		else {
			char* address = (char*)calloc(CMD_MAX, sizeof(char));
			mapaddress(command[1], address);
			flag = chdir(address);
			free(address);
		}	
	}
	findCurrentDirectory(currD);
	if(flag == -1){
		printf("Error: %s does not exist\n", command[1]);
	}
	return flag;
}

// initialise variables for history
void initialiseHistory(){
	hl = 0;
	for(int i = 0; i < 5; i++){
		history[i] = (char*)calloc(CMD_MAX, sizeof(char));
	}
}

// update the history array
void updateHistory(char* cmd){
	if(hl == 5){
		for(int i = 0; i < 4; i++){
			strcpy(history[i] , history[i+1]);
		}
		hl = 4;
	}
	strcpy(history[hl], cmd);
	hl++;
}

//print history array
void printHistory(){
	for(int i  = 0; i < hl; i++){
		printf("%s\n", history[i]);
	}
}

//execute the command return 0 if succesfull else return -1;
int executeCmd(char* command[MAX_ARG], char currD[PATH_MAX], int splitCommandLength){
	int f = fork();
	if (f < 0) {
		fprintf(stderr, "Fork failed\n");
		return -1;
	} else if (f == 0) { 
		command[splitCommandLength] = NULL;
		if(execvp(command[0], command) == -1){
			fprintf(stderr, "command does not exist\n");
		}	
		exit(1);
		return -1; 
	} else {
		int waitFlag = wait(NULL);
		return waitFlag;
	}
	return 0;
}

int main(int argc, char* argv[]){
	char currD[PATH_MAX]; // current Directory; 

	initialiseHistory();
	findCurrentDirectory(codeD);
	strcpy(currD, codeD);

	while(1){
        char* mappedCurrD = (char*)calloc(CMD_MAX, sizeof(char));
        mapToPrintAddress(currD, mappedCurrD);
		printf("MTL458:%s$ ", mappedCurrD);
		fflush(stdout);
		
		char* cmd = (char*)calloc(CMD_MAX, sizeof(char));
		size_t CmdMax = CMD_MAX;
		getline(&cmd, &CmdMax, stdin);
		cmd[129] = '\0';
		if(strlen(cmd) > 128 && cmd[128] != '\n'){
			printf("Error command length > 128\n");
			fflush(stdout);
			continue;
		}
		cmd[strlen(cmd)-1] = '\0'; // Replacing \n with \0

		updateHistory(cmd);
		char* command[MAX_ARG];
		int splitCommandLength = decodeCommand(cmd, command);
		if(splitCommandLength == -1){
			printf("Error command size  too much or wrong command\n");
			fflush(stdout);
			continue;
		}
		else{
			if(strcmp(command[0], "cd") == 0){
				executeCd(command, currD, splitCommandLength);
			}
		    else if(strcmp(command[0], "history") == 0){
		    	if(splitCommandLength > 1){
					printf("Error command size  too much or wrong command\n");
					fflush(stdout);
					continue;		    		
		    	}
		    	else{
			    	printHistory();
		    	}
		    	continue;
		    }
		    else{
		    	executeCmd(command, currD, splitCommandLength);
		    }
		}
		for(int i = 0; i < splitCommandLength; i++){
			free(command[i]);
		}
		free(cmd);
		free(mappedCurrD);
	}
	return 0;
}
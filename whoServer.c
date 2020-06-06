#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h>
#include <sys/wait.h> 
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>

typedef struct roundBuffer{
    int capacity;
    int currentSize;
    int current;
}roundBuffer;


// Initialize round buffer with given size:
roundBuffer* initializeRoundBuffer(int size)
{
    roundBuffer* newRoundBuffer = malloc(sizeof(roundBuffer));
    newRoundBuffer->capacity = size;
    newRoundBuffer->current = 0;
    newRoundBuffer->currentSize = 0;
    return newRoundBuffer;
}


int main (int argc,char* argv[]){


	int queryPortNum = 0;
	int statisticsPortNum = 0;
	int numThreads = 0;
	unsigned long int buffsize = 0;


    for(int i = 0; i < argc; i++){
        
        // strtoul converts the initial part of the string in str to an unsigned long int value according to the given base
        if(strcmp(argv[i],"-q") == 0)
        {		
            i++;
            queryPortNum = ntohs(atoi(argv[i]));
        }

        else if(strcmp(argv[i],"-s") == 0)
        {
            i++;
            statisticsPortNum = strtoul(argv[i], NULL, 10);
        }

        else if(strcmp(argv[i],"-w") == 0)
        {		
            i++;
            numThreads = strtoul(argv[i], NULL, 10);
        }

        else if(strcmp(argv[i],"-b") == 0)
        {		
            i++;
            buffsize = strtoul(argv[i], NULL, 10);
        }
       
    } 

    if(!(queryPortNum != 0 || statisticsPortNum != 0 || numThreads != NULL || buffsize != 0)){

        printf("Wrong inputs: inputs must be like './whoServer –q queryPortNum -s statisticsPortNum –w numThreads –b bufferSize' \n");  

    }



    roundBuffer* threadRoundBuffer = initializeRoundBuffer(buffsize);




}

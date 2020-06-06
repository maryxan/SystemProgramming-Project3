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
#include <time.h>
#include <sys/time.h>
//#include "functions.h"
#include <sys/socket.h>
#include <arpa/inet.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


struct data_t {
    int id;
    int sleep;

    char command[100];
};

void* thread(void *arg)
{
    struct data_t *data = arg;


    pthread_mutex_lock(&mutex);
    //printf("Thread %d: waiting for release\n", data->id);

    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex); // unlocking for all other threads

    struct timeval tv;
    gettimeofday(&tv, NULL);    

    printf("Thread %d: doing some work for %d secs, started: %ld...\n", data->id, data->sleep, tv.tv_sec);
    sleep(data->sleep);
    gettimeofday(&tv, NULL);    
    printf("Thread %d: Bye, end at %ld\n", data->id, tv.tv_sec);
}

int main (int argc,char* argv[]){

    char *queryfile = NULL;
	int servPort = 0;
	char* servIP;
	int numThreads = 0;
    struct sockaddr_in server_addr;     /* Server address */

    for(int i = 0; i < argc; i++){
        
        // Get Records File
        if(strcmp(argv[i], "-q") == 0)
        {
            i++;
            queryfile = malloc(strlen(argv[i]) * sizeof(char) + 5); // for '\0' giati to evgaze i valgrind
            strcpy(queryfile, argv[i]);
        }
        else if(strcmp(argv[i],"-w") == 0)
        {		
            i++;
            numThreads = strtoul(argv[i], NULL, 10);
        }

        else if(strcmp(argv[i],"-sp") == 0)
        {		
            i++;
            servPort = strtoul(argv[i], NULL, 10);
        }
        // Get buff size
        else if(strcmp(argv[i],"-sip") == 0)
        {
            i++;
            servIP = malloc(strlen(argv[i]) * sizeof(char) + 5); // for '\0' giati to evgaze i valgrind
            strcpy(servIP, argv[i]);
        }

       
    } 



    if(!(queryfile != NULL || servPort != 0 || servIP != NULL || numThreads != 0)){

        printf("Wrong inputs: inputs must be like './whoClient –q queryFile -numThreads –sp servPort –sip servIP' \n");  

    }


    //open the file to read it
    FILE *fp;
    fp = fopen(queryfile,"r");

    if (fp == NULL) 
    { 
        printf("Could not open file"); 
    } 

    //Start reading the file line by line
    char* line = NULL;
    size_t len = 0;

    struct data_t data[9];


    pthread_t* threads;

    threads = malloc(numThreads * sizeof(pthread_t));


    int to_create = numThreads;
    while (getline(&line, &len, fp) != EOF) {


        printf("Creating thread\n");      

        if(to_create > 0){

        //create to_create
        data[to_create].id = to_create + 1;
        data[to_create].sleep = 1 + rand() % 6;


        sscanf(line,"%s",data[to_create].command);
        // strcpy(data->command[to_create] , line);

        printf("%s\n",data[to_create].command); 
                
        pthread_create(&threads[to_create], NULL, thread, data + to_create);

        to_create--;

        }  

        // an mas teleiwsoyn ta threads stelnw prwta ta numThreads kai meta kanw kai ta upoloipa
        if(to_create == 0){

        printf("DONE CREATING %d THREADS\n",numThreads);

        // printf("making to create %d\n",numThreads );
        // give time for all threads to lock
        sleep(1);

        printf("Master: Now releasing %d threads \n",numThreads);

        pthread_cond_broadcast(&cond);

        for(int i = 0; i < numThreads; ++i)
            pthread_join((threads[i] ), NULL);

        to_create = numThreads;
        continue;

        }       
        //pthread_create(ths + i, NULL, thread, data + i);

        // to_create--;
        // printf("TO CREATE %d\n",to_create );     
    }
    printf("DONE CREATING THE THREADS\n");

    // give time for all threads to lock
    sleep(1);

    printf("Master: Now releasing the condition\n");

    pthread_cond_broadcast(&cond);

    for(int i = 0; i < numThreads; ++i)
        pthread_join((threads[i] ), NULL);


    int sockfd;
    
    //socket create and varification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    //sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd == -1) {
        perror("socket creation failed...\n");
    }

    /* Construct the server address structure */
    memset(&server_addr, 0, sizeof(server_addr));       /* Zero out structure */
    server_addr.sin_family      = AF_INET;              /* Internet address family */
    server_addr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
    server_addr.sin_port        = htons(servPort);   /* Server port */

    /* Establish the connection to the server */
    if (connect(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
        perror("connect() failed, could not find server.");

    else  printf("connected\n");

free(queryfile);
}

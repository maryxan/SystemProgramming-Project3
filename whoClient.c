#define _GNU_SOURCE
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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t dev;


typedef struct data {

   int noOlines; 
   char **command;
   int num;
   int sock;
   int port;
   char* ip;

}data;

int m = 0;
void* thread(void *d)
{
    data * temp = (data*) d;


    int j = 0;
    
    pthread_mutex_lock(&mutex);



    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex); // unlocking for all other threads

    

    // kanw lock giati i metavliti m einai koini anamesa sta threads

    pthread_mutex_lock(&mutex); 


    int sockfd;
    struct sockaddr_in server_addr;     /* Server address */
    
          
    //socket create and varification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) {
        perror("socket creation failed...\n");
    }

    /* Construct the server address structure */
    memset(&server_addr, 0, sizeof(server_addr));        /*Zero out structure*/ 
    server_addr.sin_family      = AF_INET;              /* Internet address family */
    inet_pton(AF_INET,temp->ip,&server_addr.sin_addr);/* Server IP address */
    server_addr.sin_port        = htons(temp->port);   /* Server port */

    temp->sock = sockfd;

    printf("sock is %d\n",temp->sock);

    printf("Connecting to server..\n");

    /* Establish the connection to the server */
    if (connect(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
        perror("connect() failed, could not find server.");
    else
       printf("Connection Established!\n");
    
    char buff[100]; 

    bzero(buff, sizeof(buff));

    strcpy(buff,temp->command[m]); 

    printf("Thread is sending the command: %s\n",buff);

    //write line to server
    write(temp->sock, buff, sizeof(buff)); 

    //read from server
    // bzero(buff, sizeof(buff)); 
    // read(temp->sock, buff, sizeof(buff)); 
    // printf("The asnwer from server is : %s\n", buff); 
    // // if ((strncmp(buff, "exit", 4)) == 0 && (m) == temp->num - 1) { 
    // //     printf("Client Exit...\n"); 
    // // } 
    m++;
    close(sockfd); 
    pthread_mutex_unlock(&mutex); 

}

int main (int argc,char* argv[]){

    char *queryfile = NULL;
	int servPort = 0;
	char* servIP;
	int numThreads = 0;

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
    //metraw poses grammes exei to arxeio
    char ch;
    int nolines = 0;

    while(!feof(fp))
    {
      ch = fgetc(fp);
      if(ch == '\n')
      {
        nolines++;
      }
    }
    
    nolines++;
    printf("%d\n",nolines );

    rewind(fp);

    char* line = NULL;
    size_t len = 0;

    data *d = malloc( numThreads * sizeof(data));
    d->command = malloc(nolines*sizeof(char*));
    d->noOlines = nolines;
    d->num = numThreads;
    d->sock = 0;
    d->port = servPort;
    d->ip = servIP;

    pthread_t* threads;
    threads = malloc(numThreads * sizeof(pthread_t));

    int to_create = numThreads;

    if(numThreads - nolines >0){


        to_create = nolines;

    }


    int rem = d->noOlines - numThreads;

    printf("lines %d\n",d->noOlines);
    printf("REM at start %d\n",rem );

    int pos = 0;

    //--------------------------------- create socket and connect to server ---------------------------------
    // int sockfd;
    // struct sockaddr_in server_addr;     /* Server address */
          
    // //socket create and varification
    // sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // if(sockfd == -1) {
    //     perror("socket creation failed...\n");
    // }

    // /* Construct the server address structure */
    // memset(&server_addr, 0, sizeof(server_addr));        /*Zero out structure*/ 
    // server_addr.sin_family      = AF_INET;              /* Internet address family */
    // inet_pton(AF_INET,servIP,&server_addr.sin_addr);/* Server IP address */
    // server_addr.sin_port        = htons(servPort);   /* Server port */

    // d->sock = sockfd;

    // printf("sock in struct %d\n",d->sock );

    // printf("Connecting to server..\n");

    // /* Establish the connection to the server */
    // if (connect(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
    //     perror("connect() failed, could not find server.");
    // else
    //    printf("Connection Established!\n");


    // ------------- diavazw grammi grammi to arxeio -------------------------------------------------

    int remlines = nolines;
    int created;
    while (getline(&line, &len, fp) != EOF) {
            

       
        if(to_create > 0){

            d->command[pos] = malloc((strlen(line)+1)*sizeof(char));

            strcpy(d->command[pos],line);

            pthread_create(&threads[pos], NULL, thread,(void*)d);
           
            to_create--;
            pos ++;
            remlines --;
            created;

        }  

        // an mas teleiwsoyn ta threads stelnw prwta ta numThreads kai meta kanw kai ta upoloipa
        if(to_create == 0 || remlines == 0){


          //  printf("to_create is %d\n",to_create );
            printf("DONE CREATING THREADS %d \n",numThreads);
            
            // give time for all threads to lock
            //sleep(2);

            printf("Client is now releasing %d threads \n",numThreads);

            pthread_cond_broadcast(&cond);


            for (int i = 0; i < numThreads; ++i){

                pthread_join((threads[i] ), NULL);
            }

            to_create = numThreads;
            continue;

        } 

    }

    // frees    

    for(int k=0;k<nolines;k++){

        free(d->command[k]);
        d->command[k] = NULL;
    }
    free(d->command);
    // d->command = NULL; 

    // free(d);
   // free(threads);
    //close(sockfd);
    free(queryfile);
}

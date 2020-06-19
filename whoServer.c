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
#include <sys/socket.h>       /* for AF_INET */
# include <netdb.h>
#include "namedpipes.h"

#include <poll.h>
#include <errno.h>
#include "roundbuffer.h"

int readers = 0;
int writers = 0;	

int ports[50];

pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

// Declaration of thread condition variable 
pthread_cond_t write_cond = PTHREAD_COND_INITIALIZER; 
  
// declaring mutex 
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; 

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;



typedef struct command_data {

   int noOlines; 
   char **command;
   int num;
}command_data;

//////////////////////////////////////////////////////////////////////////////////////

// Producer:

// while (true) {
//   /*produce item v */
//   pthread_mutex_lock (&M);
//   while ((in + 1) % n == out) pthread_cond_wait(&Out_CV, &M);
//   b [in] = v;
//   in = (in + 1) % n;
//   pthread_cond_signal (&In_CV);
//   pthread_mutex_unlock (&M);
// }

// Consumer:

// while (true) {
//   pthread_mutex_lock (&M);
//   while (in == out) pthread_cond_wait(&In_CV, &M);
//   w = b [out];
//   out = (out + 1) % n;
//   pthread_mutex_unlock (&M);
//   pthread_cond_signal (&Out_CV);
//   /*consume item w */
// }

/////////////////////////////////////////////////////////////////////////////////////////
	

int i = 0;
void* thread(void *temp)
{	
	FILE *fptr;

	roundBuffer * tempworker = (roundBuffer*) temp;
	
	int ret = 0;
   
	while(1){

		pthread_mutex_lock(&lock);


	    pthread_cond_wait(&cond2, &lock);

	    printf("alal\n");

	    // printf("hi\n");
	    ret = pop(tempworker,tempworker->buffer[i]);
	   	printf(" ret IS %d\n", ret);



   		char buf[100];
   		char buffcpy[100];
		int res;


		memset(buf, '\0', sizeof (buf));

        if ((res = read(ret,buf,sizeof(buf)) )< 0) {

        perror ( "problem in reading \n" ) ;

        }
        else if (res == 0)
            break;


        strcpy(buffcpy,buf);

        // an exw epikoinwnia me tous workers

        if(strcmp(strtok(buffcpy,"_"), "stats") == 0 ){


        	printf("Reading the statistics ..\n");

        	char str[20];


        	fptr = fopen(buf,"r");


        	fgets(str,6,fptr);


        	printf("str %s and %zu\n",str,strlen(str));

        	ports[i] = atoi(str);


        } 
 
        // einai o client ara stelnw queries	
        else {


        printf("client\n");
        // struct sockaddr_in addr;
		// int fd;

		// fd = socket(AF_INET, SOCK_STREAM, 0);
		// if(fd == -1)
		// {
		//     perror("Error opening socket\n");
		// }

	 //    memset(&addr, 0, sizeof(addr));        /*Zero out structure*/ 
		// addr.sin_port = htons(atoi(buf));
		// addr.sin_addr.s_addr = INADDR_ANY;
		// addr.sin_family = AF_INET;

		// // int tr=1;

  // //       // kill "Address already in use" error message
  // //        if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&tr,sizeof(int)) == -1) {
  // //           perror("setsockopt");
  // //           exit(1);
  // //       }

		// // if(bind(fd, (struct sockaddr *)&addr,sizeof(struct sockaddr_in) ) == -1)
		// // {
		// //     perror("Error binding socket\n");
		// // } 
		// // else printf("binding ok\n");

		// if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
  //       { 
  //           perror("\nConnection Failed \n");

  //       }  

  //       write(fd,buf,sizeof(buf));


        }
        printf("BUF IS %s\n",buf);

  //       printf("i is %d\n",i );

  //      

		// i++;


		

	    pthread_mutex_unlock(&lock); // unlocking 



		
	}
    

}

void* main_thread(void *roundbuffer)
{	

	printf("Main thread created.\n");

	roundBuffer * temp = (roundBuffer*) roundbuffer;

	pthread_t* threads;
    threads = malloc(temp->numThreads * sizeof(pthread_t));

    for (int i = 0; i < temp->numThreads-1; ++i)
    {
    	pthread_create(&threads[i], NULL, thread, (void*)temp);
    	sleep(1); 
    }
   	
   	// perimenw mexri na ksupnisei to main thread otan parei kati apo tin accept
   	while(1){

   	// gemizw to buffer

    pthread_mutex_lock(&lock); 


   
   	pthread_cond_wait(&write_cond, &lock);


   	int statsock = temp->fd;	
   	printf("temp->fd %d\n",temp->fd);
   	push(roundbuffer,statsock);

   	

   	pthread_cond_signal(&cond2);

   	pthread_mutex_unlock(&lock);
  

   	}


}


int main (int argc,char* argv[]){


	int queryPortNum = 0;
	int statisticsPortNum = 0;
	int numThreads = 0;
	unsigned long int buffsize = 0;
	struct sockaddr_in server, to_master , client ;
	struct sockaddr *serverptr =( struct sockaddr *) & server ;
	struct sockaddr *to_masterptr =( struct sockaddr *) & to_master ;
	struct sockaddr *clientptr =( struct sockaddr *) & client ;
	socklen_t clientlen , workerlen;
	struct hostent *rem;
	struct hostent *remstat;


	pthread_mutex_t round = PTHREAD_MUTEX_INITIALIZER;



	int newsock , statsock;

    for(int i = 0; i < argc; i++){
        
        // strtoul converts the initial part of the string in str to an unsigned long int value according to the given base
        if(strcmp(argv[i],"-q") == 0)
        {		
            i++;
            queryPortNum = (atoi(argv[i]));
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

    if(!(queryPortNum != 0 || statisticsPortNum != 0 || numThreads != 0 || buffsize != 0)){

        printf("Wrong inputs: inputs must be like './whoServer –q queryPortNum -s statisticsPortNum –w numThreads –b bufferSize' \n");  

    }

    // init ports array
    for (int i = 0; i < 50; i++)
	{
		ports[i] = 0;
				
	}	

	// init round buffer

	roundBuffer *roundbuffer = malloc(sizeof(roundBuffer));

	roundbuffer -> buffer = malloc(buffsize * sizeof(int));

	for (int i = 0; i < buffsize; ++i)
	{
		roundbuffer->buffer[i] = -1;
	}

	roundbuffer->pos = 0;
	roundbuffer->max = buffsize;
	roundbuffer->head = 0;
	roundbuffer->tail = 0;
	roundbuffer->numThreads = numThreads;
	roundbuffer->fd = 0;
    // ---------------------- create threads and round buffer ---------------------------------------------------

    command_data *data = malloc(sizeof(command_data));
    data->num = numThreads;

    //roundBuffer* r_buffer = initializeRoundBuffer(buffsize);
    // int *BufferW = malloc(sizeof(int)*buffsize);

    
    pthread_t* main_th;

    int *k;
    main_th = malloc(sizeof(pthread_t));

    pthread_create(main_th,NULL,main_thread,(void*)roundbuffer);

  

    sleep(2);
   //--------------------------------------------------Create , bind , set up Sockets ----------------------------------------------- 

    char hostbuffer[256]; 

	int socket_desc;
    int socket_desc_stats;
    int hostname;

    hostname = gethostname(hostbuffer, sizeof(hostbuffer)); 

    remstat = gethostbyname(hostbuffer);

    char *IPbuffer;
   	IPbuffer  = inet_ntoa(*((struct in_addr*) remstat->h_addr_list[0])); 

    //inet_ntoa(*((struct in_addr *)&(ip_header->saddr)))

    printf("Host IP: %s\n", IPbuffer); 

    //--------------- socket for client---------------

	if ((socket_desc = socket(AF_INET , SOCK_STREAM , 0))<0)
	{
		perror("ERROR: Could not create socket\n");
		return 3;
	}
	
	//set master socket to allow multiple connections 
    if( setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int) )){
        perror("ERROR\n");
        // exit(EXIT_FAILURE);
    }
   
	memset(&server, 0, sizeof(server));       /* Zero out structure */
	server.sin_family=AF_INET;
	//server.sin_addr.s_addr=htonl(IPbuffer);
	//inet_pton(AF_INET,IPbuffer,&server.sin_addr);/* Server IP address */
	server.sin_port=htons(queryPortNum);


	//--------------- socket for worker--------------

	if ((socket_desc_stats = socket(AF_INET , SOCK_STREAM , 0))<0)
	{
		perror("ERROR: Could not create socket\n");
		return 3;
	}

	if( setsockopt(socket_desc_stats, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int) )){
        perror("ERROR\n");

    }

    memset(&to_master, 0, sizeof(to_master));       /* Zero out structure */
	to_master.sin_family=AF_INET;
	// to_master.sin_addr.s_addr=htonl(INADDR_ANY);
	//inet_pton(AF_INET,IPbuffer,&to_master.sin_addr);/* Server IP address */
	to_master.sin_port=htons(statisticsPortNum);

	//-------------------------
	/* Bind Sockets */
	int tr=1;

    // kill "Address already in use" error message
    if (setsockopt(socket_desc,SOL_SOCKET,SO_REUSEADDR,&tr,sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

	if (bind(socket_desc,serverptr,sizeof(server))<0){
		perror("ERROR: Binding socket failed\n");
		return 3;
	}

	// kill "Address already in use" error message
    if (setsockopt(socket_desc_stats,SOL_SOCKET,SO_REUSEADDR,&tr,sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

	if (bind(socket_desc_stats,to_masterptr,sizeof(to_master))<0){
		perror("ERROR: Binding statistics socket failed\n");
		return 3;
	}
	//-----------------------------
	if (listen(socket_desc_stats,5) < 0 ){
		perror("ERROR: Linstening failed\n");
		return 3;
	}

	if (listen(socket_desc,5) < 0 ){
		perror("ERROR: Linstening failed\n");
		return 3;
	}



	printf("Listening for connections from client to port %d\n",queryPortNum);
	/* Accept incoming connections */

	printf("Listening for connections from worker(s) port %d\n",statisticsPortNum);



	// // ---------------------------------------connect to workers and get stats ---------------------------------------------------

	printf("Waiting for worker(s) and the client...\n");

	//----------------------new
	struct pollfd pfds[2];
	pfds[0].fd = socket_desc_stats;
	pfds[0].events = POLLIN;
	pfds[1].fd = socket_desc;
	pfds[1].events = POLLIN;


	//----------------------new


	// // for(int k=0;k<numThreads;k++){
	fd_set socketSet;
	int res;

	char buff[100];

	workerlen = sizeof(to_master);

	while(1){

		if(poll(pfds,2,-1) < 0){
			if (errno == EINTR)
			{
				break;
			}
			perror("poll");
            break;
		}

		// an parw request apo tous workers
		if (pfds[0].revents & POLLIN) {	


			printf("Worker connection\n");

			Stats *get_stats = malloc(sizeof(Stats));

			if( ( statsock = accept (socket_desc_stats , to_masterptr , &workerlen )) <0 ){
				perror("ERROR: Accepting connection from worker\n");
				return 3;
			}
			if ( (remstat=gethostbyaddr((char*)&to_master.sin_addr.s_addr,
				sizeof(to_master.sin_addr.s_addr), to_master.sin_family ))==NULL){
				perror("ERROR: gethostbyaddr\n");
				return 3;
			}
			printf("Connection Accepted from: %s\n",remstat->h_name);

			// adding file descriptor to round buffer

			// int i = 0;
			sleep(1);
			pthread_mutex_lock(&lock);

			printf("GOT FD %d\n",statsock);

			// while(1){

				roundbuffer->fd = statsock;

				//push(roundbuffer,roundbuffer->fd);
				// printf("apo to struct %d\n",roundbuffer->fd );

				pthread_cond_signal(&write_cond);

			// }

			
			pthread_mutex_unlock(&lock);



			//printf("Statistics sent.\n");
			 /* Wait for all threads to complete */

		}
		// an parw request apo ton client
			else {

			printf("Client connection\n");

			
			clientlen = sizeof(client);
			// accept returns a file descriptor
			if( ( newsock = accept (socket_desc , clientptr , &clientlen )) <0 ){
				perror("ERROR: Accepting connection from client\n");
				return 3;
			}
			
			printf("fd is %d\n",newsock);	
			
			if ( (rem=gethostbyaddr((char*)&client.sin_addr.s_addr,
			 sizeof(client.sin_addr.s_addr), client.sin_family ))==NULL){
				perror("ERROR: gethostbyaddr\n");
				return 3;
			}
			printf("Connection Accepted from: %s\n",rem->h_name);



			 data->command = malloc(50*sizeof(char*));
			 data->noOlines = 0;	

				int i = 0;

				// while(1){
					sleep(1);
					pthread_mutex_lock(&lock);

					printf("GOT FD %d\n",newsock);

					// while(1){

						roundbuffer->fd = newsock;

						printf("%d\n",roundbuffer->fd);

						//push(roundbuffer,roundbuffer->fd);
						// printf("apo to struct %d\n",roundbuffer->fd );

						pthread_cond_signal(&write_cond);

					// }

					
					pthread_mutex_unlock(&lock);

					// bzero(buff, sizeof(buff)); 
					// //memset(buff, '\0', sizeof (buff));

					// if ((res = read(newsock, buff, sizeof(buff)) ) < 0 ) {

					// 	perror ( "problem in reading \n" ) ;
					// 	break;
					// }
					// else if (res == 0)
				 //    	break;
				   	
				 //    data->noOlines ++;
				 //    data->command[i] = malloc((strlen(buff)+1)*sizeof(char));

				 //    strcpy(data->command[i],buff);
				 //    i++;

				 //    // print buffer which contains the client contents 
				 //    printf("message from client: %s\n", buff); 

				 //    bzero(buff, sizeof(buff)); 

					 
					// strcpy(buff,"exit");

					// sleep(1);
				 //    write(newsock, buff, sizeof(buff)); 

				// }



		}
	}	

}

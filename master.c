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
#include "namedpipes.h"
#include "list.h"
#include "preprocessing.h"
#include <sys/socket.h>
#include <arpa/inet.h>

int pstrcmp( const void* a, const void* b )
{
  return strcmp( *(const char**)a, *(const char**)b );
}

 
int main (int argc,char* argv[]){

	char* input_dir = NULL;
    unsigned long int numWorkers = 0;
    unsigned long int buffsize = 0;
    char* serverIP ;
    int serverPort = 0;
    unsigned long int per_dir = 0;

    struct sockaddr_in server_addr;     /* Server address */

    
    LinkedList* countrylist = allocate_list();
    LinkedList* diseaselist = allocate_list();


    //--------------------------------------------global stats -------------------------------------
    disease_node *head_disease = malloc(sizeof(disease_node));
    head_disease->stats = malloc(sizeof(Stats));
    strcpy(head_disease->stats->diseaseID,"HEAD");
    head_disease->next = NULL; 

    //-------------------------------------------------------------------------

    char **array = malloc(numWorkers * sizeof(disease_node));

//-------------------------------------Parse arguments from command line----------------------------------------------------

    for(int i = 0; i < argc; i++){
        
        // strtoul converts the initial part of the string in str to an unsigned long int value according to the given base
        if(strcmp(argv[i],"-w") == 0)
        {		
            i++;
            numWorkers = strtoul(argv[i], NULL, 10);
        }

        // Get buff size
        else if(strcmp(argv[i],"-b") == 0)
        {
            i++;
            buffsize = strtoul(argv[i], NULL, 10);
        }

        else if(strcmp(argv[i],"-s") == 0)
        {		
           
            i++;
            serverIP = malloc(strlen(argv[i]) * sizeof(char) + 5); // for '\0' giati to evgaze i valgrind
            strcpy(serverIP, argv[i]);
        }

        else if(strcmp(argv[i],"-p") == 0)
        {		
            i++;
            serverPort = strtoul(argv[i], NULL, 10);
        }
        //get input_dir
        else if(strcmp(argv[i],"-i") == 0)
        {
            i++;
            input_dir = malloc(strlen(argv[i]) * sizeof(char) + 5); // for '\0' giati to evgaze i valgrind
            strcpy(input_dir, argv[i]);
        }
    } 

    //check if the inputs are valid
    if(!(input_dir != NULL && numWorkers != 0 && buffsize != 0 && serverPort!=0 && serverIP !=NULL)){

        printf("Wrong inputs: inputs must be like './master –w numWorkers -b bufferSize –s serverIP –p serverPort -i input_dir' \n");  

    } else

        printf("Starting the program with the following inputs: numWorkers: %ld, buffsize: %ld, serverIP:%s, serverPort:%d, path: %s \n",numWorkers,buffsize,serverIP,serverPort,input_dir);




   //----------------------------------------metraw posa dir exei mesa to input_dir------------------------------------------------------

    struct dirent *de;  // Pointer for directory entry
    int countryNum = 0; //posa country directories exw
    int count = 0;



    DIR *dr = opendir(input_dir); //open ./input
    if (dr != NULL)  // opendir returns NULL if couldn't open directory 
    {       
        // diavazw ta dir
        while ((de = readdir(dr)) != NULL) {

        // den diavazw ta arxeia mesa   
        if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;

            else {    
                
                countryNum++;

            }
    }
    rewinddir(dr);
    closedir(dr);  
       
    } else {

        printf("Could not open current directory\n"); 
        return 0; 
    }
    
  //------------------------------------------- diaxwrismos dir stous workers--------------------------------------------------------

    int remaining = 0; // posa dir periseuoun

    per_dir = countryNum/numWorkers;  // posa paths tha parei o kathenas

    if (countryNum%numWorkers != 0 || numWorkers == 1){

        remaining = (countryNum - numWorkers); // tha elegxw mono gia >0
    }

    if(per_dir == 0){
        printf("Some workers don't take paths (num of workers:)%ld\n",(numWorkers-countryNum));
        numWorkers = countryNum; // posoi worker tha doulevoun
        per_dir = 1;
    }
     

    //------------------------------------------ ftiaxnw tous workers -------------------------------------------------------------

    pid_t childpid;
    pid_t parent = getpid(); // to ID tou gonea 
    pid_t workers[numWorkers];


    pid_t child_pid;

    for(int i=0;i<numWorkers;i++){
        workers[i] = 0;
    }

    //-------------- create fifos -----------------------
    createFIFOS(numWorkers);
    sleep(1);
    //-------------- create the workers -----------------

    int worker_port = 9000;

    for(int i = 0; i < numWorkers; i++){


        //printf("i is %d port is %d\n",i,worker_port );
                worker_port++;


        childpid = fork();

        if (childpid == -1) {
            perror("Unsuccessful fork\n");
            exit(1) ;
        } 

        //child process
        if(childpid == 0){
            workers[i] = getpid();
            printf("i is : %d .Process has as ID the number : % ld with parent id : %ld \n",i,(long)getpid(),(long)parent);
            break;
        }   
    
    }


    //if parent
    if(parent == getpid()){

        //open named pipes and read from stdin      
        struct dirent *de;
        DIR *dr = opendir(input_dir); //open ./input


        //making an array of paths and fill it with the directories
        pathsStruct* p = malloc(sizeof(pathsStruct)); 
        p->paths = malloc(countryNum*sizeof(char*));
        p->noOfPaths = countryNum;
        int i = 0;


        //---------------------------------------------------------------------------------------------------------
        //anoigw ton ./input_dir kai ftiaxnw ton pinaka me ta paths

        while ((de = readdir(dr)) != NULL) {

            // den diavazw ta arxeia mesa   
            if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 || de->d_ino == 0)
                    continue;

            else {    
                    
            char* newname = malloc(strlen(input_dir)+strlen(de->d_name)+2);
            strcpy(newname, input_dir); // Compose pathname as "name/de->d_name"
            strcat(newname, "/");
            strcat(newname, de->d_name);


            p->paths[i] = malloc((strlen(de->d_name)+1)*sizeof(char));
            strcpy(p->paths[i],newname);
            //printf("path is %s\n",p->paths[i]);
            i++;


            free(newname);


            } 

        }
        //---------------------------------tha moirasw ta paths----------------------------------------------

        //stelnw ta paths me methodo round robin
        char msg[buffsize];

        int* readfds = malloc(numWorkers*sizeof(int));
        int* writefds = malloc(numWorkers*sizeof(int));
        int* folders = malloc(numWorkers*sizeof(int));
        int folders_i;

        for(folders_i=0 ; folders_i<numWorkers ; folders_i++){
            folders[folders_i]=0;
        }

        int j = 0;
        for (int k = 0; k < numWorkers; k++){


            //char FIFO1[24]; // to read
            char FIFO2[24]; // to write

            // anoigw ta pipe gia ton kathe worker
            //sprintf(FIFO1,"Input%d",k);
            sprintf(FIFO2,"Output%d",k);

            int fd2 = -1;

            int nwrite;

            int readfd, writefd;

        
            // if((readfd = open(FIFO1, O_RDONLY)) < 0) {
            //     perror("server: can't open read fifo");
            // }
            if((writefd = open(FIFO2, O_WRONLY)) < 0) {
                perror("server: can't open write fifo");
            }

            fcntl(writefd, F_SETFL, O_NONBLOCK);
            
            //readfds[k] = readfd;
            writefds[k] = writefd;


            strcpy(msg,p->paths[j]);
            //printf("Parent is sending the path: %s\n",msg);
            folders[k]++;
            if((nwrite = write(writefds[k],msg,buffsize)) == -1){

                perror ( " Error in Writing " );
            }

            j++;

        }

        // an uparxoyn akoma monopatia enw exw kanei tin prwti moirasia
        sleep(1);
        if(remaining > 0){

            int rem = p->noOfPaths - remaining;

            for (int i = 0; i < remaining; i++)
            {
                
                strcpy(msg,p->paths[rem]);
                //printf("Parent is sending the path: %s\n",msg);


                int nwrite;
                if(numWorkers == 1){

                    folders[0]++;
                    if((nwrite = write(writefds[0],msg,buffsize)) == -1){

                        perror ( " Error in Writing " );
                    }
                }
                else{

                    folders[i%numWorkers]++;
                    if((nwrite = write(writefds[i%numWorkers],msg,buffsize)) == -1){

                        perror ( " Error in Writing " );
                    }
                }


                rem++;

            }


        }

        for (int k = 0; k < numWorkers; k++){

            int nwrite;

            sprintf(msg, "PARAMS %s %d", serverIP ,serverPort);

            //printf("Parent is sending the path: %s\n",msg);
            if((nwrite = write(writefds[k],msg,buffsize)) == -1){

                perror ( " Error in Writing " );
            }

        }
    
        // //diavazw ta statistics apo tous workers
        // Stats *get_stats = malloc(sizeof(Stats));
        // for (int i = 0; i < numWorkers; ++i){

        //     int nread,print_folder=1;
        //     char prev_date[100]="";
        //     do {
        //         if((nread = read(readfds[i],get_stats,sizeof(Stats))) < 0){
        //             perror ( " Error in Reading " );
        //         }
        //         if(print_folder==1){
        //             print_folder = 0;
        //             printf("%s\n",get_stats->countryName);
        //         }
        //         if(strcmp(get_stats->diseaseID,"ENDOFFOLDER")==0){
        //             print_folder = 1;
        //             folders[i]--;
        //             continue;
        //         }
        //         if(strcmp(prev_date,get_stats->date)!=0){
        //             printf("%s\n",get_stats->date);
        //             strcpy(prev_date,get_stats->date);
        //         }
        //         printf("%s\n",get_stats->diseaseID);
        //         printf("Age range 0-20 years: %d cases\n",get_stats->range1);
        //         printf("Age range 21-40 years: %d cases\n",get_stats->range2);
        //         printf("Age range 41-60 years: %d cases\n",get_stats->range3);
        //         printf("Age range 60+ years: %d cases\n",get_stats->range4);
        //     }while (folders[i]>0);

        // }
        // free(get_stats);
        //--------------------------------- afou stalthoun oi katalogoi tha perimenei input --------------------------  
            
        char* command;
        size_t len = 0;
        char* line = NULL;
        char* l = NULL;

        while(getline(&line,&len,stdin)){

            l = strtok(line,"\n");
            if(l == NULL){
                continue;
            }

            char copy[100];
            char *str = malloc(strlen(l) + 1);

            strcpy(str,l);
            command = strtok(str," ");

            //printf("%s\n",command);

            //works
            if(strcmp(command,"/exit")==0){

                printf("exiting\n");

                break;
            }
        } 
        free(line);

        deleteFIFOS(numWorkers);
    } // end of parent 

        //if child 
        else{
                char *IP;
                char *port;    
                char buf[buffsize];
                int res;

                int pos = returnPosWorker(numWorkers,getpid(),workers);
                
                printf("PORT %d\n",worker_port);            
                        
                //char FIFO1[24]; // to read
                char FIFO2[24]; // to write

               // sprintf(FIFO1,"Input%d", pos);
                sprintf(FIFO2,"Output%d",pos);

                        
                int readfd;


                if((readfd = open(FIFO2, O_RDONLY)) < 0)
                {
                    perror("client: can't open read fifo \n");
                }   

                /* Create Socket */

                // gia kathe worker anoigw ena socket
                int sock , valread; 
                struct sockaddr_in serv_addr; 
                struct sockaddr *serverptr =( struct sockaddr *) & serv_addr ;

                //malloc soket
                if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
                { 
                    printf("\n Socket creation error \n"); 
                    return -1; 
                } 


                /* Construct the server address structure */
                memset(&serv_addr, 0, sizeof(serv_addr));       /* Zero out structure */
                serv_addr.sin_family      = AF_INET;              /* Internet address family */
                inet_pton(AF_INET,serverIP,&serv_addr.sin_addr);/* Server IP address */
                serv_addr.sin_port        = htons(serverPort);   /* Server port */

                
                // int tr=1;

                // // kill "Address already in use" error message
                // if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&tr,sizeof(int)) == -1) {
                //     perror("setsockopt");
                //     exit(1);
                // }

                // if (bind(sock,serverptr,sizeof(serv_addr))<0){
                //     perror("ERROR: Binding failed\n");
                //     return 3;
                // }



                while(1){

                        memset(buf, '\0', sizeof (buf));

                        if ((res = read(readfd,buf,buffsize) )< 0) {

                        perror ( "problem in reading \n" ) ;
                            break;

                        }
                        else if (res == 0)
                            break;
                   
                        char cpy[100];

                        strcpy(cpy,buf);

                        if(strcmp(strtok(cpy," ") , "PARAMS" ) == 0){

                            

                            IP = strtok(NULL," ");
                            port = strtok(NULL," ");

                            worker *info = malloc(sizeof(entry));

                            strcpy(info->ip,IP);
                            strcpy(info->port,port);

                            printf("ip is %s , port is %s\n",info->ip,info->port );


                            memset(&serv_addr, 0, sizeof(serv_addr));
                            serv_addr.sin_family = AF_INET; 
                            serv_addr.sin_port = htons(serverPort); 
                            inet_pton(AF_INET,IP,&serv_addr.sin_addr);


                            printf(" connections to port %d\n",serverPort);

        

                            if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
                            { 
                                perror("\nConnection Failed \n");

                            }   

                            // for (int i = 0; i < numWorkers; ++i)
                    
                                char buff[100]; 
                                char buff1[100];

                                bzero(buff, sizeof(buff));
                                //strcpy(buff,worker_port);


                                sprintf(buff,"%d" , worker_port);

                                
                                //write line to server
                               // write(sock, buff, sizeof(buff)); 

                                strcpy(buff1,"stats");
                                printf("BUFF IS %s\n",buff1 );

                                write(sock,buff1,sizeof(buff1));
                            
                                // int red;
                                // memset(buf, '\0', sizeof (buf));

                                // if ((red = read(readfd,buf,buffsize) )< 0) {

                                // perror ( "problem in reading \n" ) ;
                                //     break;

                                // }
                                // else if (red == 0)
                                //     break;    


                            // break;

                        }
                            else{

                            ////////////////////////////////////////////////////////////////////  
                            // FEED DATA //

                            int files = 0;
                            int infiles = 0;
                            // metraw posa arxeia exei mesa o fakelos pou phre o worker
                            struct dirent *de;
                            DIR *dr = opendir(buf); //open ./input
                            while ((de = readdir(dr)) != NULL) {


                                if((de->d_type == DT_REG)){

                                    files++;

                                }
                            
                            }

                            // ftiaxnw ena pinaka me ta onomata twn arxeiwn
                            char** fileNames = malloc(files*sizeof(char*));
                            //printf("%d\n",files);

                            rewinddir(dr);
                            int k=0;
                            while((de =readdir(dr)) != NULL && k<files){
                                if((de->d_type == DT_REG)){
                                    fileNames[k] = malloc((strlen(de->d_name)+1)*sizeof(char));
                                    strcpy(fileNames[k],de->d_name);
                                    k++;

                                }
                            }


                            //tha kanw sort ton pinaka me ta files

                            //void qsort (void* base, size_t num, size_t size,int (*comparator)(const void*,const void*))

                            int size = sizeof(fileNames) / sizeof(fileNames[0]); 

                            qsort(fileNames, size, sizeof(fileNames[0]), pstrcmp);

                            // disease_node *head_disease = malloc(sizeof(disease_node));
                            // head_disease->stats = malloc(sizeof(Stats));
                            // strcpy(head_disease->stats->diseaseID,"HEAD");
                            // head_disease->next = NULL; 
                            
                            int dis;

                            // gia osa arxeia uparxoyn tha ta anoigw                
                            FILE *fp;
                            char* line = NULL;
                            size_t len = 0;

                            // for every file - open file

                            int p = numWorkers;
                            for (int i = 0; i < files; i++) {

                                char m[buffsize];
                                char chosen[100];
                                strcpy(chosen,fileNames[i]);

                                char *in = malloc(strlen(buf) + strlen(chosen) + 2);
                                *in = '\0';

                                // to kanw ws eksis : ./input_dir/Country/file.txt
                                strcat(in,buf);
                                strcat(in,"/");
                                strcat(in,chosen);

                                printf("we are opening the file: %s\n",in);
                                //feed data into structures
                                preprocessing(in,getpid(),countrylist,diseaselist,fileNames[i],head_disease);
                                free(in);


                                // if(p > 0){


                                //     array[p] = malloc(sizeof(disease_node));

                                //     strcpy(array[p],head_disease->stats);


                                //     p--;


                                // }

                                // //stats
                                // disease_node *temp_dis = head_disease->next;

                                // while (temp_dis!=NULL){
                                //     strcpy(temp_dis->stats->countryName,buf);
                                //     strcpy(temp_dis->stats->date,chosen);
                                //     // if (res= write(writefd,temp_dis->stats,sizeof(Stats) ) == 0) {
                                //     //     perror ( "problem in writing \n" ) ;
                                //     // }
                                //     //printf("hihi\n");
                                //     temp_dis=temp_dis->next;
                                // }
                                // disease_node *temp_dis2 = NULL;
                                // temp_dis = head_disease->next;
                                
                                // // while (temp_dis!=NULL){
                                // //  temp_dis2=temp_dis;
                                // //  temp_dis=temp_dis->next;
                                // //  free(temp_dis2);
                                // // }
                                
                                // head_disease->next = NULL;



                            } 



                           // free(head_disease->stats);
                           // free(head_disease);

                            // Stats *eof_stats = malloc(sizeof(Stats)) ;
                            // strcpy(eof_stats->diseaseID,"ENDOFFOLDER");
                            // if (res= write(writefd,eof_stats,sizeof(Stats) ) == 0) {
                            //     perror ( "problem in writing \n" ) ;
                            // }

                          //  free(eof_stats);

                            //free fileNames array
                            for(int k=0;k<files;k++){
                                free(fileNames[k]);
                                fileNames[k] = NULL;
                            }
                            free(fileNames);
                            fileNames = NULL;   
                            closedir(dr);


                        } //end of while




                // printf("Now listening for connections..\n");

                // if (listen(sock,5) < 0 ){
                //     perror("ERROR: Linstening failed\n");
                //     return 3;
                // }







                    
                    }
                    
            }//end of else for child



                





}    

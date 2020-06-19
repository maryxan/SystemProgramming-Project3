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
#include <netdb.h>

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
    
    // ------------------------------ files for stats -----------------------------

        FILE **filestat;
        char filename[50];
        int i;
        
        char **names = malloc(50 * sizeof(char));


        filestat = malloc( sizeof(FILE *) * 50);

        for (int i = 0; i < numWorkers; i++)
        {
            sprintf(filename,"stats_Worker%d",i);

            names[i] = malloc(20 *sizeof(char));
            strcpy(names[i],filename);

            filestat[i] = fopen(filename, "a+");


            if(filestat[i] == NULL)
            {
                printf("Error: File \"%s\" cannot be opened\n", filename);
                continue;
            } else printf("created file %s \n",filename);

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
    int position = 0;
    char n[100];

    for(int i = 0; i < numWorkers; i++){


        //printf("i is %d port is %d\n",i,worker_port );
        worker_port++;
        

        printf("%d\n",position);
        strcpy(n,names[i]);

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
    position++;
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
                struct hostent *remstat;

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

                // tou kathe worker
                int sock , serversock; 
                struct sockaddr_in serv_addr; 
                struct sockaddr *serverptr =( struct sockaddr *) & serv_addr ;


                struct sockaddr_in toserv_addr; 
                struct sockaddr *toserverptr =( struct sockaddr *) & toserv_addr ;

                if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
                { 
                    printf("\n Socket creation error \n"); 
                    return -1; 
                }



                /* Construct the server address structure */
                memset(&serv_addr, 0, sizeof(serv_addr));       /* Zero out structure */
                serv_addr.sin_family      = AF_INET;              /* Internet address family */
                inet_pton(AF_INET,serverIP,&serv_addr.sin_addr);/* Server IP address */
                serv_addr.sin_port        = htons(0);   /* Server port */


                int tr=1;

                // kill "Address already in use" error message
                 if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&tr,sizeof(int)) == -1) {
                    perror("setsockopt");
                    exit(1);
                }

                if (bind(sock,serverptr,sizeof(serv_addr))<0){
                    perror("ERROR: Binding failed\n");
                    return 3;
                }

                if (listen(sock,5) < 0 ){
                    perror("ERROR: Linstening failed\n");
                    return 3;
                }


                int len = sizeof(serv_addr);
               // getsockname(sock, (struct sockaddr *) &serv_addr, &len);

                if (getsockname(sock, (struct sockaddr *)&serv_addr, &len) == -1)
                    perror("getsockname");
                // else
                //     printf("port number %d\n", ntohs(serv_addr.sin_port));
                     
                int workerport = ntohs(serv_addr.sin_port);

                printf("%d\n",workerport );

                fprintf(filestat[position], "%d\n",workerport);


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

                            printf("Server ip is %s , Server port is %s\n",info->ip,info->port );

                            // ftiaxnw to socket pou tha sindethw ston server


                            if ((serversock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
                            { 
                                printf("\n Socket creation error \n"); 
                                return -1; 
                            }


                            memset(&toserv_addr, 0, sizeof(toserv_addr));
                            toserv_addr.sin_family = AF_INET; 
                            toserv_addr.sin_port = htons(serverPort); 
                            inet_pton(AF_INET,IP,&toserv_addr.sin_addr);/* Server IP address */

                    
                            printf(" connections to port %d\n",serverPort);

        

                            if(connect(serversock, (struct sockaddr *)&toserv_addr, sizeof(toserv_addr)) < 0) 
                            { 
                                perror("\nConnection Failed \n");

                            }   
                                char hostbuffer[256]; 

                    
                                char buff[100]; 
                                char statbuff[100];

                                // stelnw to worker port

                                strcpy(statbuff,n);

                                //write to server
                                write(serversock, statbuff, sizeof(statbuff)); 


                                printf("Woker %d is listening for connections from server to port: %d\n",position,workerport);
                                int hostname;

                                hostname = gethostname(hostbuffer, sizeof(hostbuffer)); 
                                remstat = gethostbyname(hostbuffer);

                                int statsock;
                                int len = sizeof(serv_addr);

                                if( ( statsock = accept (sock , serverptr , &len )) <0 ){
                                    perror("ERROR: Accepting connection from worker\n");
                                    return 3;
                                }
                                if ( (remstat=gethostbyaddr((char*)&serv_addr.sin_addr.s_addr,
                                    sizeof(serv_addr.sin_addr.s_addr), serv_addr.sin_family ))==NULL){
                                    perror("ERROR: gethostbyaddr\n");
                                    return 3;
                                }
                                printf("Connection Accepted from: %s\n",remstat->h_name);


                                char buff1[100];
                                int readparm;
                                memset(buff1, '\0', sizeof (buff1));

                                if ((readparm = read(statsock,buff1,sizeof(buff1)) )< 0) {

                                    perror ( "problem in reading \n" ) ;
                                        break;

                                }
                                else if (readparm == 0)
                                    break;

                                printf("I GOT %s\n",buff1 );

                                char cpy1[100];
                                char cpy2[100];
                                char cpy3[100];
                                char cpy4[100];


                                strcpy(cpy1,buff1);
                                strcpy(cpy2,buff1);
                                strcpy(cpy3,buff1);
                                strcpy(cpy4,buff1);


                                // pairnw to query kai vlepw ti einai kai apantaw


                                //works for id-name-surname-disease-age 
                                if(strcmp(strtok(buf," "),"/searchPatientRecord") == 0) 
                                {   

                                    int final = 0;
                                    char m[100];
                                    recid = strtok(NULL," ");


                                    //printf("%s\n",recid);
                                    final += searchPatientRecord(countrylist,recid);

                                    //printf("%d\n",final);
                                    sprintf(m, "%d", final);

                                    if ((res= write(writefd,m,sizeof(m))) == 0) {

                                        perror ( "problem in writing \n" ) ;
                                    }   

                                }

                                // /diseaseFrequency virusName date1 date2 [country]
                                else if(strcmp(strtok(cpy1," "),"/diseaseFrequency") == 0){

                                    int final = 0;
                                    char m[100];

                                    virus = strtok(NULL," ");
                                    parameter1 = strtok(NULL," "); 
                                    parameter2 = strtok(NULL," ");
                                    vcountry = strtok(NULL," ");

                         
                                    if (vcountry == NULL)
                                    {   

                                        
                                        final += disease_frequency(diseaselist,virus,parameter1,parameter2);

                                        sprintf(m, "%d", final);
                                        //printf("FINAL %d\n",final);

                                        if ((res= write(writefd,m,sizeof(m))) == 0) {

                                        perror ( "problem in writing \n" ) ;
                                        }
                                    }
                                    else {

                                        final += disease_frequency_with_param(diseaselist,virus,parameter1,parameter2,vcountry);
                                        sprintf(m, "%d", final);

                                        if ((res= write(writefd,m,sizeof(m))) == 0) {

                                        perror ( "problem in writing \n" ) ;
                                        }
                                    }

                                }
                                // /numPatientAdmissions disease date1 date2 [country]
                                else if(strcmp(strtok(cpy2," "),"/numPatientAdmissions") == 0) 
                                {
                                    int final = 0;
                                    char m[100];

                                    virus = strtok(NULL," ");
                                    parameter1 = strtok(NULL," "); 
                                    parameter2 = strtok(NULL," ");
                                    vcountry = strtok(NULL," ");


                                   // printf("%s %s %s %s\n",virus,parameter1,parameter2,vcountry );
                                    if (vcountry == NULL)
                                    {
                                        final += numPatientAdmissions(diseaselist,virus,parameter1,parameter2);

                                        sprintf(m, "%d", final);

                                        if ((res= write(writefd,m,sizeof(m))) == 0) {

                                        perror ( "problem in writing \n" ) ;
                                        }

                                    } else {
                                        
                                        final += numPatientAdmissionsParam(diseaselist,virus,parameter1,parameter2,vcountry);
                                        sprintf(m, "%d", final);

                                        if ((res= write(writefd,m,sizeof(m))) == 0) {

                                        perror ( "problem in writing \n" ) ;
                                        }
                                    }

                                }
                                // /numPatientDischarges disease date1 date2 [country]
                                else if(strcmp(strtok(cpy3," "), "/numPatientDischarges") == 0) 
                                {

                                    int final = 0;
                                    char m[100];

                                    virus = strtok(NULL," ");
                                    parameter1 = strtok(NULL," "); 
                                    parameter2 = strtok(NULL," ");
                                    vcountry = strtok(NULL," ");


                                    if (vcountry == NULL)
                                    {
                                        final +=numPatientDischarges(diseaselist,virus,parameter1,parameter2);
                                        sprintf(m, "%d", final);

                                        if ((res= write(writefd,m,sizeof(m))) == 0) {

                                        perror ( "problem in writing \n" ) ;
                                        }

                                    } else {

                                        final += numPatientDischargesParam(diseaselist,virus,parameter1,parameter2,vcountry);
                                        sprintf(m, "%d", final);

                                        if ((res= write(writefd,m,sizeof(m))) == 0) {

                                        perror ( "problem in writing \n" ) ;
                                        }
                                     
                                    }

                                }

                        } 
                            else{

                            printf("position to w %d\n",position);   

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

                            disease_node *head_disease = malloc(sizeof(disease_node));
                            head_disease->stats = malloc(sizeof(Stats));
                            strcpy(head_disease->stats->diseaseID,"HEAD");
                            head_disease->next = NULL; 
                            
                            int dis;

                            // gia osa arxeia uparxoyn tha ta anoigw                
                            FILE *fp;
                            char* line = NULL;
                            size_t len = 0;

                            
                            // for every file - open file
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


                                //stats
                                disease_node *temp_dis = head_disease->next;

                                while (temp_dis!=NULL){
                                    strcpy(temp_dis->stats->countryName,buf);
                                    strcpy(temp_dis->stats->date,chosen);
                                    
                                    fprintf(filestat[position], "%s\n",temp_dis->stats->date);
                                    fprintf(filestat[position], "%s\n",temp_dis->stats->countryName);
                                    fprintf(filestat[position], "%s\n",temp_dis->stats->diseaseID);
                                    fprintf(filestat[position], "Age range 0-20 years: %d cases\n",temp_dis->stats->range1);
                                    fprintf(filestat[position], "Age range 21-40 years: %d cases\n",temp_dis->stats->range2); 
                                    fprintf(filestat[position], "Age range 41-60 years: %d cases\n",temp_dis->stats->range3);
                                    fprintf(filestat[position], "Age range 60+ years: %d cases\n\n",temp_dis->stats->range4);

                                    temp_dis=temp_dis->next;

                                }
                                temp_dis = head_disease->next;
                                
                                // while (temp_dis!=NULL){
                                //  temp_dis2=temp_dis;
                                //  temp_dis=temp_dis->next;
                                //  free(temp_dis2);
                                // }
                                
                                head_disease->next = NULL;

                                


                            } 
                            if(fclose(filestat[position]) == 0){


                                printf("closed file %d\n",position );
                            }else printf("cant close file %d\n",position );

                           // free(head_disease->stats);
                           // free(head_disease);


                            //free fileNames array
                            for(int k=0;k<files;k++){
                                free(fileNames[k]);
                                fileNames[k] = NULL;
                            }
                            free(fileNames);
                            fileNames = NULL;   
                            closedir(dr);


                        } //end of while
   
                    }
                    
            }//end of else for child



                





}    

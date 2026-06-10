#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <queue.h>
#include <fcntl.h>

Queue* buffer;                            //O buffer pou tha xrhsimopoihsoume (sthn ousia einai mia oura)

pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;                  //Mutex gia thn koinh metablhth buffer
pthread_mutex_t active_workers_threads_mutex = PTHREAD_MUTEX_INITIALIZER;  //Mutex gia thn koinh metablhth twn energwn workers threads

pthread_cond_t bufferNotFull = PTHREAD_COND_INITIALIZER;                   //Condition variable gia otan to buffer den einai gemato
pthread_cond_t bufferNotEmpty = PTHREAD_COND_INITIALIZER;                  //Condition variable gia otan to buffer den einai adeio
pthread_cond_t active_workers_threads_vr = PTHREAD_COND_INITIALIZER;       ////Condition variable gia tous energous workers threads

int active_workers_threads = 0;             //H energh worker threads
int threadPoolSize;                         //O arithmos twn worker threads
int setConcurrency = 1;                    //Arithmos parallhlias
int buffersize;                            //Megethos tou buffer
int id = 1;                                //To id twn jobs
int server_socket;                         //Kai to anagnwristiko tou socket tou server mas

typedef struct {                           //Struct gia kathe Worker thread pou apothikeuoume mesa to anagnwrtiko tou socket tou client kai to id tou thread
    int clientSocket;
    pthread_t threadId;
} WorkerThreadInfo;

void* controllerThreadFunction(void* arg);
void* workerThreadFunction();

int main(int argc, char *argv[]) { 
    if (argc != 4) {                             //Elenxoume gia orthi xrhsh parametrwvn
        printf("Not enough arguments\n");
        exit(EXIT_FAILURE);
    }

    int portNum = atoi(argv[1]);                //Apothikeuoume ton arithmo ths thuras
    buffersize = atoi(argv[2]);                 //Apothikeuoume ton megethos tou buffer
    threadPoolSize = atoi(argv[3]);             //Apothikeuoume ton arithmo twn worker threads

    buffer = createQueue(buffersize);          //Dhmiourgoume to buffer mas me bash to size pou mas dothike

    server_socket = socket(AF_INET, SOCK_STREAM, 0);   //Anoigoume socket

    struct sockaddr_in server_address, client_address;   //Dhmiourgoume sockaddr_in gia ton client kai server antoistoixa
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(portNum);

    bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address));  //Kanoume bind to socket tou server
    
    listen(server_socket, 100);                    //Akoume ston server socket gia tuxon sundeseis

    WorkerThreadInfo workerThreads[threadPoolSize];  //Dhmiourgoume ta worker threads oso h h parametros threadPoolSize pou mas dothike
    for (int i = 0; i < threadPoolSize; i++) {
        pthread_create(&workerThreads[i].threadId, NULL, workerThreadFunction, NULL);
    }

    while (true) {                                    //Kai mesa se auto to while tha diabazoume gia nea sundesh kai tha thn apothexomaste kai tha dhmiourgoume controrrel thread
        socklen_t length = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &length);

        pthread_t controllerThreadId;
        pthread_create(&controllerThreadId, NULL, controllerThreadFunction, (void*)&client_socket);
        pthread_detach(controllerThreadId);
    }

    return 0;
}


void* controllerThreadFunction(void* arg) {

    int client_socket = *((int*)arg);                     //H parametros mas tha einai to anagnwristiko tou socket tou client
    char* command = (char *)malloc(64000 * sizeof(char));  //Pinakas pou tha pernoume to command apo twn client
    command[0] = '\0';
    ssize_t bytes;
    
    bytes = recv(client_socket, command, 64000, 0);       //Diabazoume ta bytes apo to socket
    command[bytes] = '\0';
    
    char *comms[1024];                                    //Apo edw kai katw xorizoume to command se tokens
    char *token;
    int i = 0;
    token = strtok(command, " ");              
    while(token != NULL) {
          comms[i++] = token;
          token = strtok(NULL, " ");
    }
    comms[i] = NULL;

   if(i!=0) {
    if (strcmp(comms[0], "issueJob") == 0) {                    //Se periptwsh pou mas dwthei issueJob command
            char* message = (char *)malloc(512 * sizeof(char)); //Afairoume apo to command to issueJob (logo ths execv)
            for(int j = 0; j < i - 1; j++) {                 
                comms[j] = comms[j + 1];
             }
                comms[i - 1] = NULL;

                Job job;                                         //Dhmiourgoume to job struct
                sprintf(job.jobID, "job_%d", id++);
                job.next = NULL;
                *job.job = malloc(strlen(*comms) + 1);
                int j;
                for(j = 0; comms[j] != NULL; j++) {
                   job.job[j] = malloc(strlen(comms[j]) + 1); 
                   strcpy(job.job[j], comms[j]); 
                }
                job.job[j] = NULL;

                job.clientSocket = client_socket;
                
                pthread_mutex_lock(&bufferMutex);
                 if (queueSize(buffer) == buffersize)                //Se periptwsh pou to buffer einai gemato tote perimenoume na paroume shma oti adeiase
                   pthread_cond_wait(&bufferNotFull, &bufferMutex);
                 enqueue(buffer, job);
                pthread_mutex_unlock(&bufferMutex);

                pthread_cond_signal(&bufferNotEmpty);              //Stelnoume shma oti den einai adeio to buffer

                char *string = NULL;                               //String gia na xeiristoume swsta to onoma tou job
                size_t length = 1;

                for (int i = 0; job.job[i] != NULL; i++) {
                    length += strlen(job.job[i]);
                    if (job.job[i + 1] != NULL) {length++;}}
                       string = (char *)malloc(length);
                       string[0] = '\0'; 
                for (int i = 0; job.job[i] != NULL; i++) {
                    strcat(string, job.job[i]);
                    if (job.job[i + 1] != NULL) {strcat(string, " ");}}
               
                sprintf(message, "JOB (%s, %s) SUBMITTED\n", job.jobID, string);
                
                send(client_socket, message, strlen(message), 0);       //Stelnoume to antoistoixo mhnuma ston client
                
                free(string);
                free(message);
            
    }
    else if (strcmp(comms[0], "setConcurrency") == 0) {                 //Se periptwsh pou mas dwthei setConcurrency command
          setConcurrency = atoi(comms[1]);
          char message[32];
          sprintf(message, "CONCURRENCY SET AT %d\n", setConcurrency);
          send(client_socket, message, strlen(message), 0);

    }
    else if (strcmp(comms[0], "stop") == 0) {                          //Se periptwsh pou mas dwthei stop command
          char message[32];
          pthread_mutex_lock(&bufferMutex);
          if(checkJob(buffer, comms[1])){                              //Tsekaroume an to job einai h oxi mesa sto buffer kai stelnoume to antoistoixo mhnuma
             sprintf(message, "JOB %s REMOVED\n", comms[1]);
             removeJob(buffer, comms[1]);
             send(client_socket, message, strlen(message), 0);
          }
          else {
             sprintf(message, "JOB %s NOT FOUND\n", comms[1]);
             send(client_socket, message, strlen(message), 0);
          }
          pthread_mutex_unlock(&bufferMutex);

    }
    else if (strcmp(comms[0], "poll") == 0) {                   //Se periptwsh pou mas dwthei poll command
          pthread_mutex_lock(&bufferMutex);
          Job* current = buffer->front;
          char message[5056];
         if(isEmpty(buffer)) {
            sprintf(message, "Buffer: Empty\n");
         }
         else {
           sprintf(message, "Buffer:\n");
           while (current != NULL) {
                char *string = NULL;                       
                size_t length = 1;
                for (int i = 0; current->job[i] != NULL; i++) {
                    length += strlen(current->job[i]);
                    if (current->job[i + 1] != NULL) {length++;}}
                       string = (char *)malloc(length);
                       string[0] = '\0'; 
                for (int i = 0; current->job[i] != NULL; i++) {
                     strcat(string, current->job[i]);
                     if (current->job[i + 1] != NULL) {strcat(string, " ");}}
                
                sprintf(message + strlen(message), "Job ID: %s, Job: %s \n", current->jobID, string);
                current = current->next;
           }
          }
          pthread_mutex_unlock(&bufferMutex);
          send(client_socket, message, strlen(message), 0);
    }
    else if(strcmp(comms[0], "exit") == 0) {                   //Se periptwsh pou mas dwthei exit command
       pthread_mutex_lock(&bufferMutex);
       if(queueSize(buffer) == 0) {                           //Tsekaroume na doume ama exoume adeio h oxi buffer gia na steiloume to antoistoixo mhnuma
           pthread_mutex_unlock(&bufferMutex);

           pthread_cond_broadcast(&bufferNotFull);
           if (active_workers_threads > 0) {                 //An exoume adeio buffer tote perimenoume na teleiwsoun ta jobs pou ektelountai dld ta energa worker threads na ginoun 0
                pthread_cond_wait(&active_workers_threads_vr, &active_workers_threads_mutex);
           }

           send(client_socket, "SERVER TERMINATED", 18, 0);

           free(command);
           close(server_socket);
           close(client_socket);
           exit(EXIT_SUCCESS);
       }  
       else if (queueSize(buffer) != 0){
           removeAllJobs(buffer);                             //Se periptwsh pou exoume akoma jobs sto buffer tote auta ta afairoume ola
           pthread_mutex_unlock(&bufferMutex);

           if (active_workers_threads > 0) {                 //Kai opws kai prin perimenoume na teleiwsoun ta jobs pou ektelountai
                pthread_cond_wait(&active_workers_threads_vr, &active_workers_threads_mutex);
           }

           send(client_socket, "SERVER TERMINATED BEFORE EXECUTION", 35, 0);
           
           free(command);
           close(server_socket);
           close(client_socket);
           exit(EXIT_SUCCESS);
       }
    }
   }

   free(command);
   pthread_exit(NULL);
}


void* workerThreadFunction() {
    while (true) {
        
        pthread_mutex_lock(&bufferMutex);
         if(queueSize(buffer) == 0)                              //An o buffer einai adeios tote tp thread apla perimenei gia shma oti o buffer dn einai adeios
            pthread_cond_wait(&bufferNotEmpty, &bufferMutex);
        pthread_mutex_unlock(&bufferMutex);

        pthread_mutex_lock(&active_workers_threads_mutex);
        if((active_workers_threads < setConcurrency) && (active_workers_threads < threadPoolSize)) {
           active_workers_threads++;                                 //Afou exoume mpei gia ektelesh job auksanoume kai to count twn energwn worker threads
           pthread_mutex_unlock(&active_workers_threads_mutex);

           pthread_mutex_lock(&bufferMutex);
              Job job = dequeue(buffer);         //Afairoume apo to buffer to job                           
           pthread_mutex_unlock(&bufferMutex);

           pthread_cond_signal(&bufferNotFull);

           pid_t pid = fork();
           if (pid == -1) {   perror("fork");  exit(EXIT_FAILURE);
           } 
           else if (pid == 0) {                                   //Edw dhmiourgoume to output file kai ta output tou job ta bazoume ekei mesa
                char file_n[20];
                sprintf(file_n, "%d.txt", getpid());
                int fd = open(file_n, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                close(fd);
                execvp(job.job[0], job.job);
                perror("execvp");
                exit(EXIT_FAILURE);
            } 
            else{                                       //Edw eimaste ston patera opou o opoios ftiaxnei katalhla to mhnuma pou tha steilh ston client kai sbhnei kai to outpu file
                int status;
                waitpid(pid, &status, 0);

                char file_n[20];
                sprintf(file_n, "%d.txt", pid);
                FILE* file = fopen(file_n, "r");
                
                char* message = malloc(64000);
                sprintf(message, "----- %s output start -----\n", job.jobID);

                char buffer[1024];
                while (fgets(buffer, sizeof(buffer), file)) {
                    strcat(message, buffer);
                }
                fclose(file);
                remove(file_n);

                strcat(message, "----- %s output end -----\n");
                send(job.clientSocket, message, strlen(message), 0);
                free(message);
            }

           pthread_mutex_lock(&active_workers_threads_mutex);
            active_workers_threads--;                         //Afou exoume teleiwsei meionoume ton counter twn energwn threads
            if(active_workers_threads == 0)                   //Kai se periptwsh pou afou meiwsoume to count kai auto gine 0 stelnoume signal (se periptwsh pou eimaste kollhmenh sto exit)
               pthread_cond_signal(&active_workers_threads_vr);
           pthread_mutex_unlock(&active_workers_threads_mutex);

        }
        else {
            pthread_mutex_unlock(&active_workers_threads_mutex);
            continue;
        }
    }
    pthread_exit(NULL);
}
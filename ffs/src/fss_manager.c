#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <time.h>
#include "sync_info_mem_store.h"
#include <sys/inotify.h>
#include <errno.h>

#define PIPE_I "fss_in"
#define PIPE_O "fss_out"

FILE *manager_log = NULL;
int in_fd, out_fd;
int workers_limit = 5;
int active_workers = 0;
Queue* queued;

#define EVENT_BUF_LEN (1000 * (sizeof(struct inotify_event) + 10))

void send_console(const char *message) {            //Sunarthsh gia na stelnei mhnuma ston console
    write(out_fd, message, strlen(message));
}

void log_manager_report(char* message){             //Sunarthsh gia na grafei to report sto manager_log kai na stelnei ston console
    fprintf(manager_log, "%s\n", message);
    fflush(manager_log); 
    //send_console(message);
}

void process_report(char *report, char *source, char* operation, pid_t pid) {  //Sunarthsh pou pernei to EXEC_REPORT apo ton worker kai to ftiaxnei sthn eputhimiti morfh
    char target[128];

    InfoLog *info = find_sync_info_mem_store(source);
    strncpy(target, info->target_dir, sizeof(target)-1);
    
    char status[32] = "";
    char details[256] = "";
    
    char *line = strstr(report, "STATUS:");
    if(line != NULL) {
        line += strlen("STATUS:");
        while(*line == ' ' || *line == '\t') line++;
        sscanf(line, "%31[^\n]", status);
    }
    
    line = strstr(report, "DETAILS:");
    if(line != NULL) {
        line += strlen("DETAILS:");
        while(*line == ' ' || *line == '\t') line++;
        sscanf(line, "%255[^\n]", details);
    }

    if (details[0] == '\0') {
        char *errors = strstr(report, "ERRORS:");
        if (errors != NULL) {
            errors += strlen("ERRORS:");
            while (*errors == ' ' || *errors == '\n') errors++;
            char *end = strchr(errors, '\n');

            if(end != NULL) {
                size_t len = end - errors;
                if (len >= sizeof(details)) len = sizeof(details) - 1;
                strncpy(details, errors, len);
                details[len] = '\0';
            } 
            else {
                strncpy(details, errors, sizeof(details) - 1);
                details[sizeof(details) - 1] = '\0';
            }
        }
    }

    if(strcmp(status, "PARTIAL") == 0) {
       char *errors = strstr(report, "ERRORS:");
       int errors_count = 0;
       if(errors != NULL) {
         errors += strlen("ERRORS:");
         char *line_ptr = errors;
          while(line_ptr != NULL && *line_ptr != '\0') {
                if(line_ptr[0] == '-') {
                    errors_count++;
                }
                line_ptr = strchr(line_ptr, '\n');
                if(line_ptr != NULL)
                    line_ptr++;  
          }
        }
        info->error_count += errors_count;   
     } 
     else if(strcmp(status, "ERROR") == 0) {
            info->error_count++;
     }

    
    char final_report[512];
    snprintf(final_report, sizeof(final_report), "[%s] [%s] [%s] [%d] [%s] [%s] [%s] \n", info->last_sync_time, source, target, (int)pid, operation, status, details);
    log_manager_report(final_report);
}

void update_info_log(InfoLog* log, char* report){                              //Sunarthsh pou enhmervnei ta stoixeia enos infolog afou teleivsei o worker gia auto to log
    update_sync_time(log);
    if(strcmp(log->status, "Running")){
       update_status(log, "Active");
    }
}

void call_worker(char *source, char *target, char *filename, char *operation) {   //Sunarthsh pou kalei ton worker
    int pipefd[2];    
    if (pipe(pipefd) == -1) {
        exit(EXIT_FAILURE);
    }                  

    pid_t pid = fork();
    if (pid == 0) {
      close(pipefd[0]); 
      dup2(pipefd[1], STDOUT_FILENO);
      close(pipefd[1]);
      
      update_status(find_sync_info_mem_store(source), "Running");
      execl("./worker", "worker", source, target, filename, operation, (char *)NULL);
      perror("execl");
      exit(EXIT_FAILURE);
     } 
    else if (pid > 0) {
       close(pipefd[1]);
       char report[1028];
       ssize_t n = read(pipefd[0], report, sizeof(report) - 1);
       if(n > 0) {
          report[n] = '\0';
          update_info_log(find_sync_info_mem_store(source), report);
          process_report(report, source, operation, pid);
          close(pipefd[0]);
       }
    }
}

void log_manager_all(char *source, char *target, int i) {                        //Sunarthsh gia na tupvnei kai na grafei sto manager_log ta basika mhnumata
    if(i == 1) {    //add: Added and Monitoring
      char log[256];
      char log_c[256] = "\0";

     snprintf(log, sizeof(log), " Added directory: %s -> %s\n", source, target);

     time_t now = time(NULL);
     struct tm *tm_info = localtime(&now);
     char timestamp[32];
     strftime(timestamp, 32, "[%Y-%m-%d %H:%M:%S]", tm_info);
     strcat(log_c, timestamp);
     strcat(log_c, log);
     fprintf(manager_log, "%s%s", timestamp, log);
     fflush(manager_log); 

     
     snprintf(log, sizeof(log), " Monitoring started for %s\n", source);
     printf("\n");

     tm_info = localtime(&now);
     char timestamp1[32];
     strftime(timestamp1, 32, "[%Y-%m-%d %H:%M:%S]", tm_info);
     strcat(log_c, timestamp1);
     strcat(log_c, log);
     fprintf(manager_log, "%s%s", timestamp1, log);
     fflush(manager_log); 
    
     printf("%s", log_c);
     send_console(log_c);

    }
    else if(i == 2) {     //cancel: stopped
        char log[1024];
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", tm_info);

        snprintf(log, sizeof(log), "%s Monitoring stopped for %s\n", timestamp, source);
        fprintf(manager_log, "%s\n" ,log);
        printf("%s\n",log);
        send_console(log);
    }

    else if(i == 3){    //status: status request
        InfoLog *info = find_sync_info_mem_store(source);
        char log[1024];
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", tm_info);

        snprintf(log, sizeof(log), "%s Status requested for %s\n"
                 "Directory: %s\n"
                 "Target: %s\n"
                 "Last Sync: %s\n"
                 "Errors: %d\n"
                 "Status: %s\n",
                 timestamp, info->source_dir,
                 info->source_dir,
                 info->target_dir,
                 strlen(info->last_sync_time) > 0 ? info->last_sync_time : "N/A",
                 info->error_count,
                 info->status);
        
        fprintf(manager_log, "%s\n" ,log);
        printf("%s\n",log);
        send_console(log);
    }
    else if(i == 4){      //sync: syncing
        char log[1024];
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", tm_info);

        snprintf(log, sizeof(log), "%s Syncing directory: %s -> %s\n", timestamp, source, target);
        fprintf(manager_log, "%s\n" ,log);
        printf("%s\n",log);
        send_console(log);
     }
     else if(i == 5){     //sync: sync complete
        char log[1024];
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", tm_info);

        snprintf(log, sizeof(log), "%s Sync completed %s -> %s Errors: %d\n", timestamp, source, target, find_sync_info_mem_store(source)->error_count);
        fprintf(manager_log, "%s\n" ,log);
        printf("%s\n",log);
        send_console(log);
     }

     else if(i == 6){     //shutdown: shutdown messages
      char log[256];
      char log_c[256] = "\0";

     snprintf(log, sizeof(log), " Shutting down manager...\n");

     time_t now = time(NULL);
     struct tm *tm_info = localtime(&now);
     char timestamp[32];
     strftime(timestamp, 32, "[%Y-%m-%d %H:%M:%S]", tm_info);
     strcat(log_c, timestamp);
     strcat(log_c, log);
     
     snprintf(log, sizeof(log), " Waiting for all active workers to finish.\n");

     tm_info = localtime(&now);
     char timestamp1[32];
     strftime(timestamp1, 32, "[%Y-%m-%d %H:%M:%S]", tm_info);
     strcat(log_c, timestamp1);
     strcat(log_c, log);

     snprintf(log, sizeof(log), " Processing remaining queued tasks.\n");

     tm_info = localtime(&now);
     char timestamp2[32];
     strftime(timestamp2, 32, "[%Y-%m-%d %H:%M:%S]", tm_info);
     strcat(log_c, timestamp2);
     strcat(log_c, log);
    
     send_console(log_c);

     }
     else if(i == 7){     //shutdown: completed
        char log[1024];
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", tm_info);

        snprintf(log, sizeof(log), "%s Manager shutdown complete.\n", timestamp);
        send_console(log);
     }
     else if(i == 8){
        InfoLog *info = find_sync_info_mem_store(source);
        char log[1024];
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", tm_info);

        snprintf(log, sizeof(log), "%s Status requested for %s\n"
                 "Directory: %s\n"
                 "Target: %s\n"
                 "Last Sync: %s\n"
                 "Errors: %d\n"
                 "Status: Inactive\n",
                 timestamp, info->source_dir,
                 info->source_dir,
                 info->target_dir,
                 strlen(info->last_sync_time) > 0 ? info->last_sync_time : "N/A",
                 info->error_count);
        
        fprintf(manager_log, "%s\n" ,log);
        printf("%s\n",log);
        send_console(log);
     }

}

void sigchld_handler(int sig) {                                                //O diaxeirisths ton shmaton apo ta forks
    pid_t pid;
    int status;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        active_workers--;  
    }
}

void process_inotify(int inotify_fd) {                                         //Sunarthsh opou na erthei shma apo thn inotify thn diaxeirizetai analogws eite na thn treksei o worker eite na thn apothikeush
    char buffer[EVENT_BUF_LEN];
    ssize_t length = read(inotify_fd, buffer, EVENT_BUF_LEN);
    if(length < 0) {
        perror("read inotify");
        return;
    }

     int i = 0;
     while(i < length) {
        struct inotify_event *event = (struct inotify_event *)&buffer[i];
        if(!(event->mask & IN_ISDIR)) {
           char operation[16] = "";
           if(event->mask & IN_CREATE) {
                strcpy(operation, "ADDED");
            } 
            else if(event->mask & IN_MODIFY) {
                strcpy(operation, "MODIFIED");
            } 
            else if(event->mask & IN_DELETE) {
                strcpy(operation, "DELETED");
            }
            
               if(active_workers < workers_limit) {
                    active_workers++;
                    call_worker(find_info_wd(event->wd)->source_dir, find_info_wd(event->wd)->target_dir, event->name, operation);
                } else {
                    Task *new_task = task_create("inotify", find_info_wd(event->wd)->source_dir, event->name, operation);
                    enqueue_entry(queued, new_task);
                }
        }
        i += sizeof(struct inotify_event) + event->len;
    }
}

int main(int argc, char *argv[]) {
    char* log_file;
    char* config_file;
    queued = create_queue();  
    
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        exit(EXIT_FAILURE);
    }
    
    int inotify_fd = inotify_init();
    if(inotify_fd < 0) { 
        perror("inotify_init"); exit(EXIT_FAILURE); 
     }


    if(argc < 5) {
     exit(EXIT_FAILURE);
    }


    //Diabazoume ta arhuments
    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "-l") == 0){
            log_file = argv[++i];
        }
        if(strcmp(argv[i], "-c") == 0){
            config_file = argv[++i];
        }
        if(strcmp(argv[i], "-n") == 0){
            workers_limit = atoi(argv[++i]);
        }
    }
    
    unlink(PIPE_I);
    unlink(PIPE_O);


    //anoigoume ta pipes
    mkfifo("fss_in", 0666);
    mkfifo("fss_out", 0666);
    
    in_fd = open("fss_in", O_RDONLY | O_NONBLOCK);
    out_fd = open("fss_out", O_WRONLY);
    
    //Anoigoume to log kai to config file
    manager_log = fopen(log_file, "a");

    FILE *fp = fopen(config_file, "r");

    char source[128], target[128];


    //Proto while loop gia tin arxikh parakolouuhsh kai sunsronismo ton zeugvn source target paths apo to config file
    while(fscanf(fp, "%s %s", source, target) == 2) {
        InfoLog* newlog = create(source, target);
        insert_sync_info_mem_store(newlog);

        int wd = inotify_add_watch(inotify_fd, source, IN_CREATE | IN_MODIFY | IN_DELETE);
        if(wd < 0) {
            perror("inotify_add_watch");
        } else {
            newlog->wd = wd;
        }
    
        if(active_workers < workers_limit) {
          active_workers++;
          call_worker(source, target, "ALL", "FULL");
        } 
        else{
            Task *new_task = task_create("add", source, NULL, NULL); 
            enqueue_entry(queued, new_task);
        }
    
        while(active_workers < workers_limit && !is_empty(queued)) {
              Task *pending = dequeue_entry(queued);
              active_workers++;
              call_worker(pending->source_dir, find_sync_info_mem_store(pending->source_dir)->target_dir, "ALL", "FULL");
        }
    }
    fclose(fp);



    char buffer[1025];
    ssize_t n;

    fd_set read_fds;
    int maxfd;
    if (in_fd > inotify_fd) {
        maxfd = in_fd;
    } else {
        maxfd = inotify_fd;
    }
    
    //Deutero loop pou ginetai h parakolouthisi toy inotifh kai epeksergasia ton mhnumaton apo to console (add, status. cancel, sync, shutdown)
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(in_fd, &read_fds);
        FD_SET(inotify_fd, &read_fds);

        int ret = select(maxfd+1, &read_fds, NULL, NULL, NULL);
        if(ret < 0) {
           perror("select");
           break;
        }

        if(FD_ISSET(inotify_fd, &read_fds)) {
            process_inotify(inotify_fd);
        }

      if(FD_ISSET(in_fd, &read_fds)) {
        memset(buffer, 0, sizeof(buffer));
        n = read(in_fd, buffer, sizeof(buffer) - 1);

        if (n > 0) {
          buffer[n] = '\0';
          buffer[strcspn(buffer, "\n")] = '\0'; 

          char command[16], source[128], target[128];
          int num_args = sscanf(buffer, "%15s %127s %127s", command, source, target);
        
          if(strcmp(command, "add") == 0 && num_args == 3) {
            if(find_sync_info_mem_store(source) == NULL){
               printf("Source Directory Does not exist\n");
               fflush(stdout);
            }
            else{
              if(strcmp(find_sync_info_mem_store(source)->target_dir, target) == 0) {
                if((strcmp(find_sync_info_mem_store(source)->status, "Active") == 0)  ||  (strcmp(find_sync_info_mem_store(source)->status, "Running")) == 0){
                   char response[512];
                   time_t now = time(NULL);
                   struct tm *tm_info = localtime(&now);
                   char timestamp[32];
                    strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", tm_info);
        
                    snprintf(response, sizeof(response), "%s Already in queue: %s\n", timestamp, source);
                    send_console(response);
                }
                else{
                    int wd = inotify_add_watch(inotify_fd, source, IN_CREATE | IN_MODIFY | IN_DELETE);
                    find_sync_info_mem_store(source)->wd = wd;
                    if(active_workers < workers_limit){
                      log_manager_all(source, target, 1);
                       active_workers++;
                      printf("%d\n", active_workers);
                      call_worker(source, target, "ALL", "FULL");
                      update_status(find_sync_info_mem_store(source), "Active");
                      find_sync_info_mem_store(source)->active = 1;
                    }
                    else{
                      Task *new_task = task_create("add", source, NULL, NULL); 
                      enqueue_entry(queued, new_task);
                    }
                }
              }
              else{
                printf("Wrong target directory for the: %s", source);
                fflush(stdout);
              }
            }

         } 
         else if (strcmp(command, "status") == 0 && num_args >= 2) {
            if(find_sync_info_mem_store(source)->active){      
                log_manager_all(source, target, 3);
            } 
            else {
                log_manager_all(source, target, 8);
            }

         } 
         else if (strcmp(command, "cancel") == 0 && num_args >= 2) {
            InfoLog *info = find_sync_info_mem_store(source);
            if (info->active) {
                inotify_rm_watch(inotify_fd, find_sync_info_mem_store(source)->wd);
                find_sync_info_mem_store(source)->wd = 0;
                update_status(find_sync_info_mem_store(source), "Stopped");
                find_sync_info_mem_store(source)->active = 0;
                log_manager_all(source, target, 2);
            } 
            else {
                char response[512];
                time_t now = time(NULL);
                struct tm *tm_info = localtime(&now);
                char timestamp[32];
                strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", tm_info);

                snprintf(response, sizeof(response), "%s Directory not monitored: %s \n", timestamp, info->source_dir);
                send_console(response);
            }

         } 
         else if (strcmp(command, "sync") == 0 && num_args >= 2) {
            if(strcmp(find_sync_info_mem_store(source)->status, "Stopped") == 0){
                find_sync_info_mem_store(source)->active = 1;
                int wd = inotify_add_watch(inotify_fd, source, IN_CREATE | IN_MODIFY | IN_DELETE);
                find_sync_info_mem_store(source)->wd = wd;
            }

            if((strcmp(find_sync_info_mem_store(source)->status, "Running")) == 0){
                char response[512];
                time_t now = time(NULL);
                struct tm *tm_info = localtime(&now);
                char timestamp[32];
                strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", tm_info);

                snprintf(response, sizeof(response), "%s  Sync already in progress: %s\n", timestamp, source);
                send_console(response);
            }
            else{
               log_manager_all(source, find_sync_info_mem_store(source)->target_dir, 4);
               if(active_workers < workers_limit){
                active_workers++;
                call_worker(source, find_sync_info_mem_store(source)->target_dir, "ALL", "FULL");
                log_manager_all(source, find_sync_info_mem_store(source)->target_dir, 5);
               }
               else{
                 Task *new_task = task_create("sync", source, "null", "null"); 
                 enqueue_entry(queued, new_task);
                }
            }

         } 
         else if (strcmp(command, "shutdown") == 0) {
            log_manager_all(source, target, 6);
            while(exist_running_task() || !is_empty(queued)) {
              while(active_workers < workers_limit && !is_empty(queued)) {
                Task *pending = dequeue_entry(queued);
                active_workers++;
                if(strcmp(pending->command, "inotify") == 0){
                    call_worker(pending->source_dir, find_sync_info_mem_store(pending->source_dir)->target_dir, pending->filename, pending->operation);
                }
                if(strcmp(pending->command, "sync") == 0){
                    call_worker(pending->source_dir, find_sync_info_mem_store(pending->source_dir)->target_dir, "ALL", "FULL");
                    log_manager_all(pending->source_dir, find_sync_info_mem_store(pending->source_dir)->target_dir, 5);
                }
                if(strcmp(pending->command, "add") == 0){
                    call_worker(pending->source_dir, find_sync_info_mem_store(pending->source_dir)->target_dir, "ALL", "FULL");
                 }
              }
              continue;
            }
            log_manager_all(source, target, 7); 
            break;
         } 
       } 

      } 
      else {
        usleep(100000);
      }

      while(active_workers < workers_limit && !is_empty(queued)) {
         Task *pending = dequeue_entry(queued);
         if(strcmp(pending->command, "inotify") == 0) {
           active_workers++;
           call_worker(pending->source_dir, find_sync_info_mem_store(pending->source_dir)->target_dir, pending->filename, pending->operation);
         }
         else{
           active_workers++;
           call_worker(pending->source_dir, find_sync_info_mem_store(pending->source_dir)->target_dir, "ALL", "FULL");
           if(strcmp(pending->command, "sync") == 0){
             log_manager_all(source, target, 5);
           }
         }   
      }
   }

    close(in_fd);
    close(out_fd);
    free_store();
    free_store_queue(queued);
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>
#include <time.h>

#define PIPE_I "fss_in"
#define PIPE_O "fss_out"

FILE *console_log = NULL;

void log_console(char *message, int i){      //Sunarthsh pou eite tupwnei kai grafei sto console_lo file eite mono grafei
   if(i == 1){
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, 32, "[%Y-%m-%d %H:%M:%S]", tm_info);
    fprintf(console_log, "%s %s %s\n", timestamp, "Command", message);
   } 
   else if(i == 2){
    fprintf(console_log, "%s\n", message);
    fflush(console_log); 
    printf("%s\n", message);
   }
}

int main(int argc, char *argv[]) {
    char command[512], message[512];
    int in_fd, out_fd;
    char* log_file;

    if(argc < 3) {
        exit(EXIT_FAILURE);
     }
    
    //diabazoume ta arguments
    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "-l") == 0){
            log_file = argv[++i];
        }
    }

    in_fd = open(PIPE_I, O_WRONLY);
    if(in_fd == -1) {
        perror("pipe in");
        exit(EXIT_FAILURE);
    }

    out_fd = open(PIPE_O, O_RDONLY | O_NONBLOCK);
    if(out_fd == -1) {
        perror("pipe out");
        exit(EXIT_FAILURE);
     }
   
    console_log = fopen(log_file, "a");

    fd_set read_fds;
    int max_fd = (STDIN_FILENO > out_fd) ? STDIN_FILENO : out_fd;
    
    //While sto opoio diabazoume kai stelnoume sunexeia mhnumata me ton manager
    printf("> "); 
    fflush(stdout); 
    while(1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(out_fd, &read_fds);
      
        int ret = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if(ret < 0) {
            perror("select");
            break;
        }
      
        if(FD_ISSET(out_fd, &read_fds)) {
            printf("\n");    
            fflush(stdout); 
            int n = read(out_fd, message, sizeof(message) - 1);
            if(n > 0) {
                message[n] = '\0';
                log_console(message, 2);
                if(strstr(message, "Manager shutdown complete") != NULL) {
                    break;
                }
                printf("> ");    
                fflush(stdout); 
            }
        }
      
        if(FD_ISSET(STDIN_FILENO, &read_fds)) {
            //printf("> ");    
            //fflush(stdout); 
            if(fgets(command, sizeof(command), stdin) != NULL) {
                log_console(command, 1);
                write(in_fd, command, strlen(command));
            }
        }
    }

    close(in_fd);
    close(out_fd);
}

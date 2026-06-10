#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define ERR_BUF_SIZE 1024

int copy(char *source, char *target) {            //Sunarthsh pou kanei copy ta arxeia apo to source sto target directory
    int source_fd = open(source, O_RDONLY);
    if(source_fd < 0) {
        return -1;
    }

    int target_fd = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(target_fd < 0) {
        close(source_fd);
        return -1;
    }
    
    char buffer[3000];
    ssize_t bytes_read, bytes_written;
    
    while((bytes_read = read(source_fd, buffer, sizeof(buffer))) > 0) {
           bytes_written = write(target_fd, buffer, bytes_read);
            if(bytes_written != bytes_read) {
              close(source_fd);
               close(target_fd);
              return -1;
            }
     }
    
     if(bytes_read < 0) {
        close(source_fd);
        close(target_fd);
        return -1;
     }
    
     close(source_fd);
     close(target_fd);
     return 0;
}

void make_path(char *dest, size_t dest_size, char *dir, char *file) {
    snprintf(dest, dest_size, "%s/%s", dir, file);
}

int main(int argc, char *argv[]) {
    if(argc != 5) {
        exit(EXIT_FAILURE);
    }
    
    //Diabazoume ta arguments
    char *source = argv[1];
    char *target = argv[2];
    char *filename = argv[3];
    char *operation = argv[4];
    
    printf("EXEC_REPORT_START\n");
    
    int returnn = 0;
    int files_copied = 0, files_skipped = 0;
    char details[256] = "";
    char status_result[32] = "SUCCESS";
    char error_buffer[1024];
    error_buffer[0] = '\0';
    

    //Analoga se poia periptvsei eimaste enhmervnoume to details kai to error_buffer
    if(strcmp(operation, "FULL") == 0) {
     if(strcmp(filename, "ALL") == 0) {
            DIR *dir = opendir(source);
             if(!dir) {
                snprintf(error_buffer + strlen(error_buffer), ERR_BUF_SIZE - strlen(error_buffer), "- Fail '%s': %s\n", source, strerror(errno));
                strcpy(status_result, "ERROR");
                returnn = -1;
             } 
             else 
             {
                struct dirent *entry;
                while ((entry = readdir(dir)) != NULL) {
                    if(entry->d_type == DT_REG) {
                       char source_path[100], target_path[100];
                        make_path(source_path, sizeof(source_path), source, entry->d_name);
                        make_path(target_path, sizeof(target_path), target, entry->d_name);
                        if(copy(source_path, target_path) == 0) {
                            files_copied++;
                        } 
                         else {
                            files_skipped++;
                            snprintf(error_buffer + strlen(error_buffer), ERR_BUF_SIZE - strlen(error_buffer),"- File: %s - %s\n", entry->d_name, strerror(errno));
                        }
                    }
                }
                closedir(dir);
                if(files_skipped > 0 && files_copied > 0) {
                    snprintf(details, sizeof(details), "%d files copied, %d skipped", files_copied, files_skipped);
                    strcpy(status_result, "PARTIAL");
                }    
                else if(files_copied == 0 && files_skipped > 0){
                    snprintf(details, sizeof(details), "%d files skipped", files_skipped);
                    strcpy(status_result, "ERROR");
                }
                else if(files_copied > 0 && files_skipped == 0){
                    snprintf(details, sizeof(details), "%d files copied", files_copied);
                }
            }
        } 
        else {
            returnn = -1;
        }
    }
    else if(strcmp(operation, "ADDED") == 0 || strcmp(operation, "MODIFIED") == 0) {
        char source_path[100], target_path[100];
        make_path(source_path, sizeof(source_path), source, filename);
        make_path(target_path, sizeof(target_path), target, filename);
        if(copy(source_path, target_path) == 0) {
            snprintf(details, sizeof(details), "File: %s", filename);
            files_copied = 1;
        } 
        else {
            snprintf(error_buffer + strlen(error_buffer), ERR_BUF_SIZE - strlen(error_buffer), "- File: %s - %s\n", filename, strerror(errno));
            strcpy(status_result, "ERROR");
            returnn = -1;
        }
    }
    else if(strcmp(operation, "DELETED") == 0) {
        char target_path[100];
        make_path(target_path, sizeof(target_path), target, filename);
        if(unlink(target_path) == 0) {
            snprintf(details, sizeof(details), "File: %s", filename);
        } 
        else {
            snprintf(error_buffer + strlen(error_buffer), ERR_BUF_SIZE - strlen(error_buffer), "- File: %s - %s\n", filename, strerror(errno));
            strcpy(status_result, "ERROR");
            returnn = -1;
        }
    }
    
    printf("STATUS: %s\n", status_result);
    printf("DETAILS: %s\n", details);

    if(strlen(error_buffer) > 0) {
        printf("ERRORS:\n%s", error_buffer);
    } else {
        printf("ERRORS:\nNone\n");
    }
    printf("EXEC_REPORT_END\n");
    
    exit(returnn == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

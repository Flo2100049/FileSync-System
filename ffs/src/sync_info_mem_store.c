#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sync_info_mem_store.h"
#include <stdbool.h>

////////////////////////////////////////////// Gia thn kentrikh apothikeush ///////////////////////////
InfoLog* create(char *source, char *target) {
    InfoLog *newlog = (InfoLog*)malloc(sizeof(InfoLog));
    if (!newlog) {
        exit(EXIT_FAILURE);
    }
    strncpy(newlog->source_dir, source, sizeof(newlog->source_dir) - 1);
    newlog->source_dir[sizeof(newlog->source_dir) - 1] = '\0';
    
    strncpy(newlog->target_dir, target, sizeof(newlog->target_dir) - 1);
    newlog->target_dir[sizeof(newlog->target_dir) - 1] = '\0';
    
    strcpy(newlog->status, "Active");
    newlog->last_sync_time[0] = '\0'; 
    newlog->active = 1;
    newlog->error_count = 0;
    newlog->wd = 0;
    newlog->next = NULL;
    
    return newlog;
}


InfoLog *sync_info_mem_head = NULL;


void insert_sync_info_mem_store(InfoLog *newlog) {
    if (sync_info_mem_head == NULL) {
        sync_info_mem_head = newlog;
    } else {
        InfoLog *current = sync_info_mem_head;
        while (current->next != NULL)
            current = current->next;
        current->next = newlog;
    }
}


InfoLog* find_sync_info_mem_store(char *source) {
    InfoLog *current = sync_info_mem_head;
    while (current != NULL) {
        if (strcmp(current->source_dir, source) == 0)
            return current;
        current = current->next;
    }
    return NULL;
}


void update_sync_time(InfoLog *info) {
    if (info) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(info->last_sync_time, sizeof(info->last_sync_time), "%Y-%m-%d %H:%M:%S", tm_info);
    }
}


void update_status(InfoLog* info, char* status) {
    strcpy(info->status, status);
}


void print_sync_info_mem_store() {
    InfoLog *current = sync_info_mem_head;
    while (current != NULL) {
        printf("Source: %s | Target: %s | Status: %s | Last Sync: %s | Active: %d | Errors: %d\n",
            current->source_dir, current->target_dir, current->status,
            (strlen(current->last_sync_time) > 0 ? current->last_sync_time : "N/A"),
            current->active, current->error_count);
        current = current->next;
    }
}


void free_store() {
    InfoLog *current = sync_info_mem_head;
    while (current != NULL) {
        InfoLog *temp = current;
        current = current->next;
        free(temp);
    }
    sync_info_mem_head = NULL;
}


bool exist_running_task(){
    InfoLog *current = sync_info_mem_head;
    while (current != NULL) {
        if((strcmp(current->status, "Running")) == 0){
           return true;
        }
        current = current->next;
    }  
   return false;     
}


InfoLog* find_info_wd(int wd) {
    InfoLog *current = sync_info_mem_head;
    while (current != NULL) {
        if (current->wd == wd)
            return current;
        current = current->next;
    }
    return NULL;
}



////////////////////////////////////////////// oura gia tiw queed ///////////////////////////////////
Task* task_create(char* command, char* source, char* filename, char* operation){
    Task *newtask = (Task*)malloc(sizeof(Task));
    if (!newtask) {
        exit(EXIT_FAILURE);
    }
    strncpy(newtask->command, command, sizeof(newtask->command) - 1);
    newtask->command[sizeof(newtask->command) - 1] = '\0';
    

    strncpy(newtask->source_dir, source, sizeof(newtask->source_dir) - 1);
    newtask->source_dir[sizeof(newtask->source_dir) - 1] = '\0';

    strncpy(newtask->filename, filename, sizeof(newtask->filename) - 1);
    newtask->filename[sizeof(newtask->filename) - 1] = '\0';

    strncpy(newtask->operation, operation, sizeof(newtask->operation) - 1);
    newtask->operation[sizeof(newtask->operation) - 1] = '\0';
    
    newtask->next = NULL;

    return newtask;
}


Queue* create_queue() {
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    queue->front = queue->rear = NULL;
    queue->size = 0;
    return queue;
}


void enqueue_entry(Queue *queue, Task *task) {
    if (queue->rear == NULL) {
        queue->front = queue->rear = task;
    } else {
        queue->rear->next = task;
        queue->rear = task;
    }
    queue->size++;
}


Task* dequeue_entry(Queue *queue) {
    if (queue->front == NULL) {
        return NULL; 
    }

    Task *task = queue->front;
    queue->front = queue->front->next;

    if (queue->front == NULL) {
        queue->rear = NULL; 
    }

    task->next = NULL;
    queue->size--;
    return task;
}


bool is_empty(Queue *queue) {
    if(queue_size(queue) == 0)
      return true;
    else
      return false;
}


Task* first_task(Queue *queue) {
    return queue->front;
}


int queue_size(Queue *queue) {
    return queue->size;
}


void free_store_queue(Queue* queue) {
    Task* current = queue->front;
    Task* next;
    while(current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    queue->front = NULL;
    queue->rear = NULL;
}

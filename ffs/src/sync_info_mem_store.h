#define SYNC_INFO_MEM_STORE
#include <stdbool.h>

typedef struct InfoLog {
    char source_dir[128];     
    char target_dir[128];      
    char status[10];         //Status tha einai eite Active eite Running eite Stopped  
    char last_sync_time[32];   
    int active;                
    int error_count;     
    int wd;                 //descriptor gia thn inotify     
    struct InfoLog *next;    
} InfoLog;

extern InfoLog *sync_info_mem_head;

InfoLog* create(char *source, char *target);
void insert_sync_info_mem_store(InfoLog *newlog);
InfoLog* find_sync_info_mem_store(char *source);
void update_sync_time(InfoLog *info);
void update_status(InfoLog* info, char* status);
void print_sync_info_mem_store();
bool exist_running_task();
InfoLog* find_info_wd(int wd);
void free_store();

typedef struct Task {
    char command[32];       //apothikeuoume mesa thn entolh apo to consolo "add, sync" h an erthei shma apo inotify grafoyme apla inotify
    char source_dir[128];  
    char filename[128];     //se periptosh entolhs 
    char operation[32];     //apo to inotify apothikeuoyme to filename kai to operation
    struct Task *next; 
} Task;

typedef struct Queue {
    Task *front;   
    Task *rear;    
    int size;         
} Queue;

Task* task_create(char* command, char* source, char* filename, char* operation);
Queue* create_queue();
void enqueue_entry(Queue *queue, Task *task);
Task* dequeue_entry(Queue *queue);
bool is_empty(Queue *queue);
Task* first_task(Queue *queue);
int queue_size(Queue *queue);
void free_store_queue(Queue *queue);

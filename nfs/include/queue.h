#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct Job {
    int clientSocket;
    char jobID[32];
    char *job[256];
    struct Job* next;
} Job;

typedef struct Queue {
    Job* front;
    Job* rear;
    int position;
    int maxSize;
} Queue;

Queue* createQueue(int size);
void destroyQueue(Queue* queue);
void enqueue(Queue* queue, Job data);
Job dequeue(Queue* queue);
int queueSize(Queue* queue);
void removeJob(Queue* queue, const char* jobId);
bool checkJob(Queue* queue, const char* jobId);
void removeAllJobs(Queue* queue);
bool isFull(Queue* queue);
bool isEmpty(Queue* queue);

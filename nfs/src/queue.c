#include <queue.h>

Queue* createQueue(int size) {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue->front = queue->rear = NULL;
    queue->position = 0;
    queue->maxSize = size + 1;
    return queue;
}

void destroyQueue(Queue* queue) {
    while (queue->front != NULL) {
        Job* temp = queue->front;
        queue->front = queue->front->next;
        free(temp);
    }
    free(queue);
}

bool isFull(Queue* queue) {
    return queue->position == queue->maxSize;
}

bool isEmpty(Queue* queue) {
    return queue->position == 0;
}

void enqueue(Queue* queue, Job data) {
    if (isFull(queue)) {
        return;
    }

    Job* job = (Job*)malloc(sizeof(Job));
    job->clientSocket = data.clientSocket;
    strcpy(job->jobID, data.jobID);
    *job->job = malloc((strlen(*data.job)) * sizeof(char*));
     int j = 0;
     for(j = 0; data.job[j] != NULL; j++) {
         job->job[j] = malloc(strlen(data.job[j]) + 1); 
         strcpy(job->job[j], data.job[j]); 
     }
    job->job[j] = NULL;
    
    job->next = NULL;

    if (queue->rear == NULL) {
        queue->front = queue->rear = job;
    } else {
        queue->rear->next = job;
        queue->rear = job;
    }
    queue->position++;
}

Job dequeue(Queue* queue) {
    if (isEmpty(queue)) {
        exit(EXIT_FAILURE);
    }

    Job* temp = queue->front;
    Job data = *queue->front;
    queue->front = queue->front->next;
    free(temp);

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    queue->position--;
    return data;
}

int queueSize(Queue* queue) {
    return queue->position;
}

bool checkJob(Queue* queue, const char* jobId) {
    Job* current = queue->front;
    while (current != NULL) {
        if (strcmp(current->jobID, jobId) == 0) {
            return true;
        }
        current = current->next;
    }
    return false;
}

void removeJob(Queue* queue, const char* jobId) {
    Job* previous = NULL;
    Job* current = queue->front;
    while (current != NULL) {
        if (strcmp(current->jobID, jobId) == 0) {
            if (previous == NULL) {
                queue->front = current->next;
                if (queue->front == NULL) {
                    queue->rear = NULL;
                }
            } else {
                previous->next = current->next;
                if (current->next == NULL) {
                    queue->rear = previous;
                }
            }
            free(current);
            queue->position--;
            break;
        }
        previous = current;
        current = current->next;
    }
}

void removeAllJobs(Queue* queue) {
    Job* current = queue->front;
    while (current != NULL) {
         removeJob(queue, current->jobID);
         current = current->next;
    }
}
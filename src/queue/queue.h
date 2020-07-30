//
// Created by cedric on 17/7/20.
//

#ifndef EMBSYS_EX2_QUEUE_H
#define EMBSYS_EX2_QUEUE_H

#include <pthread.h>
#include <sys/time.h>
#include <stdbool.h>
#include <stdlib.h>

//#define QUEUESIZE 10    // Size of queue

// Work item struct
typedef struct {
    void * (*work)(void *);
    void * arg;
}workFunction;

// Queue struct
typedef struct {
    int head, tail;
    int full, empty;
    int queueSize;
    bool prodEnd;   // Variable to signify end of production - used to exit consumers
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
    workFunction** buf;
} queue;

// Function Signatures for Queue and WorkFunction struct functions
queue *queueInit (int queueSize);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction* in);
void queueDel (queue *q, workFunction **out);
workFunction *workFunctionInit (void * (*workFunc)(void *), void * arg);

#endif //EMBSYS_EX2_QUEUE_H

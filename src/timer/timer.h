#ifndef EMBSYS_EX2_TIMER_H
#define EMBSYS_EX2_TIMER_H

#include <pthread.h>
#include <sys/time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "../csv/csv.h"
#include "../queue/queue.h"

// Timer is a pointer to a timer_struct
typedef struct timer_struct* timer;

// timer_struct
struct timer_struct{

    // Timer period (msec)
    int period;

    // Number of times to execute the timer
    int tasksToExecute;

    // Initial delay in seconds before starting timer execution
    int startDelay;

    // Arguments passed to producerFcn
    void* userData;

    // Function pointer for function that runs before
    void (*startFcn)();

    // Function pointer for function that runs after timer's last iteration
    void (*stopFcn)();

    // Function pointer to the producer function
    void* (*producerFcn)(void *);

    // Function pointer to function that runs when queue full
    void (*errorFcn)();

    // PThreads thread started by the timer
    pthread_t *producerThread;

};

// Timer Constructor
timer newTimer(int period, int tasksToExecute, void* (*producerFcn)(void *), void* userData, int startDelay,
               void (*startFcn)(), void (*stopFcn)(), void (*errorFcn)());

// Function used to start the timer
int startTimer(timer timer_obj);

// Function used to start the timer at a specific date and time
int startTimerAt(timer timer_obj, int y, int m, int d, int h, int min, int sec);

// Delete timer
int deleteTimer(timer timer_obj);

#endif //EMBSYS_EX2_TIMER_H

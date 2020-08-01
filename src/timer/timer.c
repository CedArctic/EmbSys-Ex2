#include "timer.h"

// Timer constructor
timer newTimer(int period, int tasksToExecute, void* (*producerFcn)(void *), void* userData, int startDelay,
               void (*startFcn)(), void (*stopFcn)(), void (*errorFcn)()){

    // Write Struct
    timer self = (timer)calloc(1, sizeof(struct timer_struct));
    self->period = period;
    self->tasksToExecute = tasksToExecute;
    self->producerFcn = producerFcn;
    self->userData = userData;
    self->startDelay = startDelay;
    self->startFcn = startFcn;
    self->stopFcn = stopFcn;
    self->errorFcn = errorFcn;

    // Start and detach thread
    self->producerThread = calloc(1, sizeof(pthread_t));

    return self;
}

// Start the timer
int startTimer(timer timer_obj){

    // Create a thread and detach it
    int error = pthread_create(timer_obj->producerThread, NULL, timer_obj->producerFcn, timer_obj);
    if(error){
        return 1;
    }
    error = pthread_detach(*(timer_obj->producerThread));
    if(error){
        return 2;
    }
    return 0;
}

// Start the timer at a specific time
int startTimerAt(timer timer_obj, int y, int m, int d, int h, int min, int sec){

    // Convert target time to epoch
    struct tm runTime_tm;
    time_t runTime_tod, curTime;

    runTime_tm.tm_year = y - 1900;  // Year - 1900
    runTime_tm.tm_mon = m - 1;           // Months start at 0 so minus 1
    runTime_tm.tm_mday = d;               // Day of the month
    runTime_tm.tm_hour = h;
    runTime_tm.tm_min = min;
    runTime_tm.tm_sec = sec;
    runTime_tm.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
    runTime_tod = mktime(&runTime_tm);

    // Get current time and calculate difference
    time(&curTime);
    long int diff = runTime_tod - curTime;

    // Add difference to initial delay
    if(diff > 0){
        timer_obj->startDelay += diff;
    }

    // Start timer
    startTimer(timer_obj);

    return 0;
}

// Delete a timer object
int deleteTimer(timer timer_obj){
    free(timer_obj->producerThread);
    free(timer_obj);
    return 0;
}
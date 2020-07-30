#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include "src/queue/queue.h"
#include "src/timer/timer.h"
#include "src/csv/csv.h"

// ===== Configuration Parameters ======

// Number of consumer threads
#define CONSUMERS 3

// Number of jobs for each timer to run. Timers: 1s, 100ms, 10ms
#define T1S_JOBS        3600
#define T100MS_JOBS     36000
#define T10MS_JOBS      360000
#define QUEUESIZE       3

// ===== Function Signatures ======

// Function Signatures - void pointer as argument and as return type
void *producer (void *timer_arg);
void *consumer (void *q);

// Function to run input function for 10 different arguments
double tenfold(double (*functionPtr)(double), double* args);

// ===== Global variables =====

// Queue wait time for jobs, statistics array and mutex to safely write it
int queueWaitStatsPtr = 0;
int *queueWaitStats;
pthread_mutex_t queueWaitStatsMut;

// Lost work counter (jobs that didn't enter the queue because it was full)
int queueFullCounter = 0;

// Completed consumer and producer threads counter
int compConThreads = 0;
int compProdThreads = 0;

// Mutex used to safely increment the finished producers/timers counter
pthread_mutex_t compProdThreadsMut;

// Mutex used to safely increment the full queue counter
pthread_mutex_t queueFullMut;

// An accumulator variable for the results of tenfold() - mainly to circumvent compiler optimizations that would skip work
long tenfoldAcc = 0;

// ===== Function Definitions =====

void printTime(){

    char buffer[30];
    struct timeval tv;

    time_t curtime;

    gettimeofday(&tv, NULL);
    curtime=tv.tv_sec;

    strftime(buffer,30,"%m-%d-%Y  %T.",localtime(&curtime));
    printf("%s%ld\n",buffer,tv.tv_usec);
}

void taskEnd(){
    printf("Timer ended\n");
}

void taskStart(){
    printf("Timer started\n");
}

void queueFull(){
    queueFullCounter++;
    printf("Queue Full\n");
}

// Calculates the minimum safe sleep time for the main thread
int sleepTime(){

    // Get the maximum run time (ms) of the 3 counters
    int max = (T1S_JOBS * 1000 > T100MS_JOBS * 100)? T1S_JOBS * 1000 : T100MS_JOBS * 100;
    max = (T10MS_JOBS * 10 > max)? T10MS_JOBS * 10 : max;

    // Convert to seconds
    max = (int)ceil((double)max/1000);

    return max;
}

// Entry Point
int main(){

    // Print starting time
    printf("Starting time ");
    printTime();

    // Create results arrays
    queueWaitStats = calloc(T1S_JOBS + T100MS_JOBS + T10MS_JOBS, sizeof(int));

    // Create a queue
    queue *fifo;
    fifo = queueInit (QUEUESIZE);
    if (fifo ==  NULL) {
        fprintf (stderr, "main: Queue Init failed.\n");
        exit (1);
    }

    // Create and run Consumer threads
    pthread_t *con = calloc(CONSUMERS, sizeof(pthread_t));
    for(int j = 0; j < CONSUMERS; j++){
        pthread_create (con + j, NULL, consumer, fifo);
    }

    // Create and start producer timers
    timer t1s, t100ms, t10ms;
    if(T1S_JOBS > 0){
        t1s = newTimer(1000, T1S_JOBS, &producer, fifo, 0, &taskStart, &taskEnd, &queueFull);
        startTimer(t1s);
    }

    if(T100MS_JOBS > 0){
        t100ms = newTimer(100, T100MS_JOBS, &producer, fifo, 0, &taskStart, &taskEnd, &queueFull);
        startTimer(t100ms);
    }

    if(T10MS_JOBS > 0){
        t10ms = newTimer(10, T10MS_JOBS, &producer, fifo, 0, &taskStart, &taskEnd, &queueFull);
        startTimer(t10ms);
    }

    // Wait (seconds) until producers finish production
    sleep(sleepTime());

    // Sleep while timers/producers haven't completed
    while(compProdThreads < ((T1S_JOBS > 0) + (T100MS_JOBS > 0) + (T10MS_JOBS > 0))){
//        printf("Sleeping while waiting for completion");
        sleep(30);
    }

    // Signal end of production
    fifo->prodEnd = true;

//    printf("Joining consumers");
    // Join consumer threads
    while (compConThreads < CONSUMERS){
        pthread_cond_broadcast(fifo->notEmpty);
    }
    for(int i = 0; i < CONSUMERS; i++){
        pthread_join (con[i], NULL);
    }

    // Write Statistics to file
    writeCSV("timeInQueue.csv", queueWaitStats, 1, T1S_JOBS + T100MS_JOBS + T10MS_JOBS, 1);
    writeCSV("queueFullCounter.txt", &queueFullCounter, 1, 1, 1);

    // Print time and accumulator
//    printf("Time: ");
//    printTime();
    printf("\nAccumulator is %lu\nProcess complete...\n", tenfoldAcc);

    // Delete queue, free queueWaitStats, consumers and timers
    free(queueWaitStats);
    queueDelete (fifo);
    free(con);

    if(T1S_JOBS > 0){
        deleteTimer(t1s);
    }

    if(T100MS_JOBS > 0){
        deleteTimer(t100ms);
    }

    if(T10MS_JOBS > 0){
        deleteTimer(t10ms);
    }

    return 0;
}

// Producer Function. Receives a pointer to the timer structure used to start it.
void *producer (void *timer_arg)
{
    // Cast timer_arg to timer object
    timer timer_obj = timer_arg;

    // Structures to calculate time
    struct timeval prevTime, curTime, prodEndTime;

    // Cumulative queue lag
    int queueLag = 0;

    // Time between two consecutive executions
    int timeBetween = 0;

    // Time spent on a single producer iteration
    int producerTime = 0;

    // queueLag, timeBetween and producerTime statistics
    int* queueLagStats = calloc(timer_obj->tasksToExecute, sizeof(int));
    int* timeBetweenStats = calloc(timer_obj->tasksToExecute, sizeof(int));
    int* producerTimeStats = calloc(timer_obj->tasksToExecute, sizeof(int));

    // Cast queue were work will be inserted
    queue *fifo;
    fifo = timer_obj->userData;

    // Array of 6 trigonometric function pointers
    double (*funcArr[6])(double) = {&sin, &cos, &tan, &acos, &asin, &atan};

    // Temporary character buffer
    char tempBuffer[50];

    // Wait for the initial time delay
    sleep(timer_obj->startDelay);

    // Run startFcn before starting execution
    timer_obj->startFcn();

    // Initialize prevTime
    gettimeofday(&prevTime, NULL);

    // Seed random numbers generation
    srand(time(NULL));

    // Run tasksToExecute number of times
    for(int i=0; i < timer_obj->tasksToExecute; i++){

        // Get current iteration time
        gettimeofday(&curTime, NULL);

        // ==== Run producer function tasks ====

        // Random arguments array
        double* randArgs;

        // Fill random args array
        randArgs = calloc(10, sizeof(double ));
        int randArgBase = rand()%40;
        for(int j = 0; j < 10; j++){
            randArgs[j] = (randArgBase + j * 5) * 3.14/180;
        }

        // Create workFunction struct. Select a random trig function and create array of random arguments
        workFunction* w = workFunctionInit((void*(*)(void*))funcArr[rand() % 6], randArgs);

        // Acquire lock to queue
        pthread_mutex_lock (fifo->mut);

        // If queue is not full, add work. Else run error function
        if (fifo->full == 0) {
            // Write work enqueuing time and add it to queue
            gettimeofday(((void**)(w->arg))[1], NULL);
            queueAdd (fifo, w);
            pthread_mutex_unlock (fifo->mut);
            pthread_cond_signal (fifo->notEmpty);
        }else{
            // Release queue lock
            pthread_mutex_unlock (fifo->mut);
            // Acquire lock to lost job counter, increment it and release the lock
            pthread_mutex_lock (&queueFullMut);
            timer_obj->errorFcn();
            pthread_mutex_unlock (&queueFullMut);
        }

        gettimeofday(&prodEndTime, NULL);

        // ==== End of producer function tasks ====

        // Calculate time difference between current and previous execution and add it to the cumulative queueLag
        if(i > 0) {
            timeBetween = (curTime.tv_sec - prevTime.tv_sec) * 1000000 + (int) (curTime.tv_usec - prevTime.tv_usec);
            queueLag += timeBetween - timer_obj->period * 1000; // usec
        }

//        printf("Iteration: %d, QueueLag: %d\n", i, queueLag);

        // Copy curTime to prevTime
        prevTime = curTime;

        // Calculate producer iteration time
        producerTime = (prodEndTime.tv_sec - curTime.tv_sec) * 1000000;      // sec to usec
        producerTime += (prodEndTime.tv_usec - curTime.tv_usec);   // Add usec

        // Write queueLag, timeBetween and producerTime statistics
        queueLagStats[i] = queueLag;
        timeBetweenStats[i] = timeBetween;
        producerTimeStats[i] = producerTime;

        // Check for missed tick and handle it by not sleeping and proceeding to the next execution
        if((timer_obj->period*1000 - queueLag) < 0){
            continue;
        }

        // Wait appropriate time (period-queueLag) with usleep
        if(queueLag > 0){
            usleep(timer_obj->period*1000 - queueLag);
        }else{
            usleep(timer_obj->period*1000);
        }

    }

    // Run stopFcn before exiting
    timer_obj->stopFcn();

    // Write queueLagStats, timeBetweenStats and producerTimeStats to CSV
    sprintf(tempBuffer, "queueLagStats-timer%d.csv", timer_obj->period);
    writeCSV(tempBuffer, queueLagStats, 1, timer_obj->tasksToExecute, 1);
    sprintf(tempBuffer, "timeBetweenStats-timer%d.csv", timer_obj->period);
    writeCSV(tempBuffer, timeBetweenStats, 1, timer_obj->tasksToExecute, 1);
    sprintf(tempBuffer, "producerTimeStats-timer%d.csv", timer_obj->period);
    writeCSV(tempBuffer, producerTimeStats, 1, timer_obj->tasksToExecute, 1);

    // Free memory
    free(queueLagStats);
    free(timeBetweenStats);
    free(producerTimeStats);

    // Increment completed producer threads counter
    pthread_mutex_lock(&compProdThreadsMut);
    compProdThreads++;
    pthread_mutex_unlock(&compProdThreadsMut);

    return NULL;
}

// Consumer Function
void *consumer (void *q)
{
    // Cast received argument as a queue
    queue *fifo;
    fifo = (queue *)q;

    // Variable to hold consumed item
    workFunction* w;

    // End timer
    struct timeval endTime;
    int queueWaitTime;

    // Consume
    while(1){
        // Get lock
        pthread_mutex_lock (fifo->mut);
        while ((fifo->empty) && (fifo->prodEnd == false)) {
            //printf ("consumer: queue EMPTY.\n");
            pthread_cond_wait (fifo->notEmpty, fifo->mut);
        }
        // Check for end of production and an empty work queue
        if ((fifo->prodEnd) && (fifo->empty)){
            compConThreads++;
            pthread_mutex_unlock (fifo->mut);
            break;
        }

        // Take an item off the queue
        queueDel (fifo, &w);

        // End fifo critical section
        pthread_mutex_unlock (fifo->mut);
        pthread_cond_signal (fifo->notFull);

        // Get time stamp of the moment the job is out of the queue
        gettimeofday(&endTime, NULL);

        // Parse work arguments
        void** args = w->arg;
        double* workArgs = (double*)args[0];

        // Run the work
        tenfoldAcc += tenfold((double (*)(double))w->work, workArgs);

        // Calculate and write the workFunctions' waiting time in the queue to the results array
        struct timeval *startTime = (struct timeval*) args[1];
        queueWaitTime = (endTime.tv_sec - startTime->tv_sec) * 1000000;      // sec to usec
        queueWaitTime += (endTime.tv_usec - startTime->tv_usec);   // Add usec

        // Write statistics
        pthread_mutex_lock(&queueWaitStatsMut);
        queueWaitStats[queueWaitStatsPtr] = queueWaitTime;
        queueWaitStatsPtr++;
        pthread_mutex_unlock(&queueWaitStatsMut);

        // Free memory of consumed item
        free(startTime);
        free(workArgs);
        free(args);
        free(w);
    }
    return NULL;
}

// Tenfold function: Receives a trig function pointer and angle arguments executes them
double tenfold(double (*functionPtr)(double), double* args){
    double sum = 0;
    for(int i = 0; i < 10; i++){
        sum += (double)(*functionPtr)(args[i]);
    }
    return sum;
}

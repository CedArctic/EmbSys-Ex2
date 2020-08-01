#include "queue.h"

// Queue Constructor
queue *queueInit (int queueSize)
{
    queue *q;

    q = (queue *)malloc (sizeof (queue));
    if (q == NULL) return (NULL);

    q->buf = calloc(queueSize, sizeof(workFunction*));
    q->queueSize = queueSize;
    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->prodEnd = false;
    q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
    pthread_mutex_init (q->mut, NULL);
    q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notFull, NULL);
    q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
    pthread_cond_init (q->notEmpty, NULL);

    return (q);
}

// Queue destructor
void queueDelete (queue *q)
{
    pthread_mutex_destroy (q->mut);
    free (q->mut);
    pthread_cond_destroy (q->notFull);
    free (q->notFull);
    pthread_cond_destroy (q->notEmpty);
    free (q->notEmpty);
    free (q);
}

// Add element to queue in a cyclic buffer
void queueAdd (queue *q, workFunction* in)
{
    q->buf[q->tail] = in;
    q->tail++;
    if (q->tail == q->queueSize)
        q->tail = 0;
    if (q->tail == q->head)
        q->full = 1;
    q->empty = 0;

}

// Remove element from queue
void queueDel (queue *q, workFunction **out)
{
    *out = q->buf[q->head];

    q->head++;
    if (q->head == q->queueSize)
        q->head = 0;
    if (q->head == q->tail)
        q->empty = 1;
    q->full = 0;
}

// Initializer for workFunction
workFunction *workFunctionInit (void * (*workFunc)(void *), void * arg)
{
    // Create work struct
    workFunction *w;
    w = (workFunction *)calloc(1, sizeof (workFunction));
    if (w == NULL) return (NULL);

    // Get creation time
    struct timeval *startTime = calloc(1, sizeof(struct timeval));
    gettimeofday(startTime, NULL);

    // Write work struct
    void** finalArgs = calloc(2, sizeof(void*));
    finalArgs[0] = arg;
    finalArgs[1] = startTime;
    w->work = workFunc;
    w->arg = finalArgs;

    return w;
}
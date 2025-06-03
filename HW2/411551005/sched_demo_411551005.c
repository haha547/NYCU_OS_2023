#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define MAX_THREAD_NUMBER 10

extern char *optarg;
extern int optind, opterr, optopt;

pthread_barrier_t start_barrier;

typedef struct {
    pthread_t thread_id;
    int thread_num;
    int sched_policy; // [FIFO, NORMAL] = [SCHED_FIFO, SCHED_OTHER]
    int sched_priority;
    double time_wait;
} thread_info_t;

void parse_arguments(int argc, char *argv[], int *num_threads, double *time_wait, int **policies, int **priorities) {
    int opt;
    char *schedulers_csv = NULL;
    char *priorities_csv = NULL;

    while ((opt = getopt(argc, argv, "n:t:s:p:")) != -1) {
        switch (opt) {
        case 'n':
            *num_threads = atoi(optarg);
            break;
        case 't':
            *time_wait = atof(optarg);
            break;
        case 's':
            schedulers_csv = optarg;       
            break;
        case 'p':
            priorities_csv = optarg;
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s -n num_threads -t time_wait -s policies -p priorities\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Allocate memory for policies and priorities arrays
    *policies = malloc(*num_threads * sizeof(int));
    *priorities = malloc(*num_threads * sizeof(int));

    // Parse the schedulers and priorities
    char *token;
    int index = 0;

    // Parse schedulers
    token = strtok(schedulers_csv, ",");
    while (token != NULL && index < *num_threads) {
        if (strcmp(token, "NORMAL") == 0) {
            (*policies)[index] = SCHED_OTHER;
        } else if (strcmp(token, "FIFO") == 0) {
            (*policies)[index] = SCHED_FIFO;
        } else {
            fprintf(stderr, "Invalid scheduler policy: %s\n", token);
            exit(EXIT_FAILURE);
        }
        token = strtok(NULL, ",");
        index++;
    }

    // Parse priorities
    token = strtok(priorities_csv, ",");
    index = 0;
    while (token != NULL && index < *num_threads) {
        (*priorities)[index] = atoi(token);
        token = strtok(NULL, ",");
        index++;
    }
}

void busy_wait(double wait_seconds) {
    struct timespec start, now;
    double elapsed_seconds;

    clock_gettime(CLOCK_MONOTONIC, &start);

    do {
        clock_gettime(CLOCK_MONOTONIC, &now);
        elapsed_seconds = (now.tv_sec - start.tv_sec);
        elapsed_seconds += (now.tv_nsec - start.tv_nsec) / 1000000000.0;
    } while (elapsed_seconds < wait_seconds);
}

void *thread_func(void *arg) {
    thread_info_t *tinfo = (thread_info_t *)arg;

    // Wait until all threads are ready
    pthread_barrier_wait(&start_barrier);

    // Do the task
    for (int i = 0; i < 3; i++) {
        printf("Thread %d is running\n", tinfo->thread_num);

        // Busy for <time_wait> seconds
        busy_wait(tinfo->time_wait);
    }

    // Exit the function
    return NULL;
}

void set_thread_affinity(pthread_t thread, int cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);

    int ret = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}

int main(int argc, char *argv[]) {
    int num_threads;
    double time_wait;
    int *policies = NULL;
    int *priorities = NULL;
    int cpu_id = 0; // Assuming you want to bind threads to CPU 0

    /* 1. Parse program arguments */
    parse_arguments(argc, argv, &num_threads, &time_wait, &policies, &priorities);

    // Initialize the barrier for the number of threads + the main thread
    pthread_barrier_init(&start_barrier, NULL, num_threads + 1);

    // Allocate memory for the thread_info_t structs
    thread_info_t *tinfo = calloc(num_threads, sizeof(thread_info_t));

    pthread_attr_t attr;
    struct sched_param param;

    /* 2. Create <num_threads> worker threads with specific policies and priorities */
    for (int i = num_threads - 1; i >= 0; i--)  {
        // Initialize thread attribute
        pthread_attr_init(&attr);
        
        //printf("policies[%d] = %d priority = %d \n", i, policies[i], priorities[i]);

        // Set the scheduling policy in the attribute structure
        pthread_attr_setschedpolicy(&attr, policies[i]);

        // Set the scheduling priority in the attribute structure only if it's not SCHED_NORMAL
        if (policies[i] != SCHED_OTHER) {
            param.sched_priority = priorities[i];
            pthread_attr_setschedparam(&attr, &param);

            // Ensure the thread attributes are applied
            pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        }

        tinfo[i].thread_num = i ;// Thread numbering starts at 0
        tinfo[i].time_wait = time_wait;
        tinfo[i].sched_policy = policies[i];
        tinfo[i].sched_priority = priorities[i];
        

        // Create thread with the attributes
        int ret = pthread_create(&tinfo[i].thread_id, &attr, &thread_func, &tinfo[i]);

        set_thread_affinity(tinfo[i].thread_id, cpu_id);

        // Destroy the thread attribute object
        pthread_attr_destroy(&attr);
    }

    // Wait for all threads to be ready
    pthread_barrier_wait(&start_barrier);

    // Join all threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(tinfo[i].thread_id, NULL);
    }
    
    return 0;
}

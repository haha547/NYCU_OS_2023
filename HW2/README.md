Scheduling Policy Demonstration Program
===

NYCU Operation System aut 2023 Homework2

* [Assignment 2: Scheduling Policy Demonstration Program](https://hackmd.io/@Bmch4MS0Rz-VZWB74huCvw/rJ8OLx6fp)

### How I implemented the program 
> Describe how you implemented the program in detail.


The main thread's implementation can be broken down into six distinct stages:

1. Parsing Program Arguments

    Utilize `getopt` to interpret command line options and `strtok` to split and identify each thread's scheduling policy and priority.
    ```c
    pthread_barrier_wait(&start_barrier);
    ```

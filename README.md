# NYCU_OS_2023
Course: Operating Systems (2023)  
Instructor: Prof. 吳俊峯, National Yang Ming Chiao Tung University

## HW1: Custom System Calls
- Implement a basic **"Hello World" system call** in the Linux kernel.
- Develop a system call that **reverses a given string**, demonstrating how to pass and manipulate data between user space and kernel space.

## HW2: Thread Scheduler Implementation
- Implement and experiment with various **CPU scheduling policies** in the Linux kernel.

  - **Fair Scheduling Policies:**
    - `SCHED_NORMAL`: Default time-sharing scheduler.
    - `SCHED_BATCH`: Optimized for background tasks that do not require interactive response.

  - **Real-Time Scheduling Policies:**
    - `SCHED_FIFO`: First-In-First-Out real-time scheduler.
    - `SCHED_RR`: Round-Robin real-time scheduler with time-slice enforcement.

## HW3: Linux Kernel Module Development
- Create a **Linux kernel module** to interact with and retrieve system-level information, such as CPU or memory status.
- Gain hands-on experience with **kernel programming**, including module insertion, removal, and logging kernel messages.


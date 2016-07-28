# Word-Count

Same word-count program with different architectures and Linux IPC mechanisms. Each version of the program targets a different Linux IPC and program architecture

The folder called "Word-Count Algorithm" is a straight-forward program there is nothing interesting. It is subset of other projects in this repository

The folder called "parent-child processes" is a program that has parent-child relationship between processes it creates. Communication via parent-child processes is done via unnamed pipes.

The folder called "Named Pipes (FIFO)" is a program which has client-server architecture with named pipes(FIFOs) to enable communcation between the client and the server processes and double fork technique to avoid zombie processes within the server.

The folder named "POSIX SharedMem and Semaphores" is exactly the same as "Named Pipes (FIFO)" except that it uses POSIX shared memory and semaphores for communication.

# Word-Count

Same word-count program with different architectures and Linux IPC mechanisms Each version of the program targets a different Linux IPC and program architecture

First version is straight-forward program there is nothing interesting.

Second version includes parent-child relationship among processes plus unnamed pipes to practice interprocess communication.

Third version has client-server architecture with named pipes(FIFOs) and double fork technique to avoid zombie processes.

Fourth version is exactly the same as the third one except that it uses POSIX shared memory and semaphores for communication.

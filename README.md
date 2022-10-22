# You Spin Me Round Robin

This program simulates a round robin scheduling algorithm on a given set of processes with a specified quantum length.

## Building

To build the program, enter the lab2 directory, which should contain the Makefile and rr.c, and execute the command "make".

## Running

To run the program, use the following command structure: "./rr FILEPATH QUANTUM_LENGTH", where FILEPATH is a path to a file that specifies the processes. The program should output the average waiting and response times of the processes. For example, "./rr processes.txt 3" should output "Average waiting time: 7.0" and "Average response time: 2.75".

## Cleaning up

To clean up the binary files, run the command "make clean" in the lab2 directory.

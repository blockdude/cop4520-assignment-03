# Results and Documentation

To ensure efficiency and proper load balancing I used an atomic
integer to pick out which present to insert at that time.

To ensure correctness I used mutex locks and condition variables.
I used the mutex lock and condition varaible together to make
sure that shared memory is safe and to notify threads when it is
their time to write to the shared memory.

I ran my program multiple times and expected to see different
times for each run and I did. I also checked values of variables
to make sure presents where linking correctly.


# Problem 1

How I solved this is by implementing a coarse-grained singly
linked list. This allowed each servant to have access to the list
and modify it without any fear of other servants modifying the
list at the same time.


# Problem 2

I created a shared memory pool where each sensor would write to
and would make sure that data was correctly written to by using
mutex locks.

Progress is guaranteed by having an atomic variable count the
time elapsed. After each minute, each sensor will be notified to
begin checking the temperature. After, the main thread will wait
for the sensors to finish reading the temperature.


# Building and Compiling

To compile you will need GCC and optionally Make.

## Method 1: using GCC

To compile using GCC navigate to directory where the source file
is then run the command:

> gcc parta.cpp -lstdc++ -lm -o parta  
> gcc partb.cpp -lstdc++ -lm -o partb  

Or 

> g++ parta.cpp -o parta  
> g++ partb.cpp -o partb  

This should generate a binary file named "main".

## Method 2: using Make

To use Make to build this program first navigate to the directory
with the source file then run the command:

> make

This should generate a two binary files named "parta" and "partb".


# Running

To run the program use:

> ./parta  
> ./partb  

Or if you are using Make:

> make run

This should run both programs and print to standard output if the
program ran successfully or not.

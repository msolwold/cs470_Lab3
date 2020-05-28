# CS470 Lab3

## Introduction

This README is for a program that uses a user-specified number of threads to sort a given number of integers in a text file. 

The number of threads, M, are provided via user input and the number of integers to sort, N, are provided via command line argument.

For every active thread, it generates a randomly selected (i, j) pair where $i <= j <= N$.
That thread will then attempt to retrieve the values from index i to index j. If there is a thread that is currently processing the values from index i to index j, the thread will generate a new (i, j) pair as to not conflict with the work of the first thread. 

Once the thread has an (i,j) pair that is not conflicting, it will randomly select a sorting algorithm to sort the values. The possible sorting algorithms are listed below. Once sorted, the thread will write the values back into the file in the same location that it retrieved them from.

A single thread, watcher, will periodically parse the file to check if the file has been sorted. Once the entire file is sorted, the watcher thread will set the global variable 'isSorted' to 1 denoting that the file is sorted and the threads can exit.

## Requirements

This program is meant to be ran on a **macOS** system. Although, I am not aware of conflicts on other Operating Systems.

## Instructions

To run this program open a terminal and navigate to the directory where the program Exists.

After compiling main.c on a macOS system, run the program from the command line while providing a number of integers for the program to generate.

The program will then prompt the user to provide a number of threads to create.

There are no known limitations given enough time for the program to run its course.

The results will be saved in a text file named "randomnumbers.txt".

## Sorting Algorithms

This program uses QuickSort, Insertion Sort, and Bubble Sort. All three of these algorithms were implemented based on code from GeeksForGeeks.

* Quick Sort: https://www.geeksforgeeks.org/quick-sort/
* Insertion Sort: \\url\{https://www.geeksforgeeks.org/insertion-sort/
* Bubble Sort: https://www.geeksforgeeks.org/bubble-sort/

## Mutex Locked Portions of the program

This program utilizes 2 mutex locks:

### fileLock
This lock is required to avoid any conflicts with reading and writing from the file. Unfortunately, this locks the entire file leading to synchronized access to the file.

### sortingLock
This lock handles the acquisition of (i, j) pairs. Every thread will generate an (i, j) pair and then attempt to get the sortingLock. Once the thread has the lock, it references the array described below that keeps track of which areas of the file are being processed. If there are no conflicts, the thread makes the range (i, j) as in-use and then releases the lock. If there are conflicts, the lock is released and the thread generates a new (i, j) pair.

## Road Blocks

Initially, the lab required region locking of a file. It was recommended that we use fcntl(), a POSIX function that dealt with region locking on files. The goal of this was to have threads sort distinct regions of the file instead of using a mutex lock that would create a synchronous process.

After extensive research into fcntl(), it was discovered that it had not been created with threads in mind. The article linked below discusses this.

{https://www.samba.org/samba/news/articles/low_point/tale_two_stds_os2.html}

There are macros that avoid the issues described in the article, but they were created under the GNU Project and only available to GNU systems.

I was able to implement a very crude version of region locking with an array. If an index was included in a range that was being processed by a thread, its value is 1 in the array. Otherwise, its value is 0. When a thread generates an (i, j) pair, it checks this array for conflicts. If there are none, it sets the range from i to j in the array to 1.

## Error Handling

This program will exit if an invalid, or missing, number of integers is provided.

Once a valid file is opened, the program will prompt the user for a number of threads. Response should be a positive integer. All other inputs will be rejected and the user will be requested to re-enter their response.
/**
 * Michael Solwold
 * CS470
 * Lab3
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h> /* :( */
#include <unistd.h>
#include <string.h>

/* CONSTANTS */

#define NUM_THREADS 15
#define FILE_NAME "randomnumbers.txt"

struct thread_args
{
    int numEls;
    int start;
    int end;
    int tid;
};

pthread_mutex_t fileLock;
pthread_mutex_t sortingLock;

/* GLOBALS */

int num_values;
int fileIsSorted = 0;
int *iLOCKED;
int *jLOCKED;

/**
 * SORTING ALGORITHMS
 *
 * DISCLAIMER:
 *
 *  These algorithms were obtained from GeeksforGeeks.com
 *  in order to focus time on implementation of thread management.
 *
 *  Yes. I know how these sorting algorithms work.
 */

// A utility function to swap two elements
void swap(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

/* This function takes last element as pivot, places
   the pivot element at its correct position in sorted
    array, and places all smaller (smaller than pivot)
   to left of pivot and all greater elements to right
   of pivot */
int partition(int arr[], int low, int high)
{
    int pivot = arr[high]; // pivot
    int i = (low - 1);     // Index of smaller element

    for (int j = low; j <= high - 1; j++)
    {
        // If current element is smaller than the pivot
        if (arr[j] < pivot)
        {
            i++; // increment index of smaller element
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

/* The main function that implements QuickSort
 arr[] --> Array to be sorted,
  low  --> Starting index,
  high  --> Ending index */
void quickSort(int arr[], int low, int high)
{
    if (low < high)
    {
        /* pi is partitioning index, arr[p] is now
           at right place */
        int pi = partition(arr, low, high);

        // Separately sort elements before
        // partition and after partition
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

// A function to implement bubble sort
void bubbleSort(int arr[], int n)
{
    int i, j;
    for (i = 0; i < n - 1; i++)

        // Last i elements are already in place
        for (j = 0; j < n - i - 1; j++)
            if (arr[j] > arr[j + 1])
                swap(&arr[j], &arr[j + 1]);
}

/* Function to sort an array using insertion sort*/
void insertionSort(int arr[], int n)
{
    int i, key, j;
    for (i = 1; i < n; i++)
    {
        key = arr[i];
        j = i - 1;

        /* Move elements of arr[0..i-1], that are
          greater than key, to one position ahead
          of their current position */
        while (j >= 0 && arr[j] > key)
        {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
}

/* HELPER FUNCTIONS */

int getRandomNum(int min, int max)
{
    return (rand() % ((max - min) + 1)) + min;
}

/**
 * Parse command line arguments
 * @param argc
 * @param argv
 * @return 0 if invalid, number of elements to generate if valid
 */
int parseCLI(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Number of elements not provided. Please provide the number of elements you would like generated.\n");
        return 0;
    }

    int n = atoi(argv[1]);
    if (n <= 0)
    {
        printf("Invalid number of elements given. Please provide a non-zero integer.\n");
        return 0;
    }
    return n;
}

/**
 * Get number of threads from user
 *
 * @param n
 * @return number of threads to create
 */
int getUserInput(int n)
{
    char input[5];
    int m = 0;
    do
    {
        printf("Enter # of threads to sort %d numbers: ", n);
        fgets(input, 5, stdin);
        m = atoi(input);
        if (m <= 0)
        {
            printf("Invalid number of threads given. Please enter a non-zero integer.\n");
        }
        else
            break;
    } while (1);

    return m;
}

/**
 * Fills file with n random numbers from 0 - maxValue
 *
 * @param file
 * @param n amount of numbers
 * @param maxValue Upper bound of numbers
 */
void populateFile(int maxValue)
{
    FILE *file;

    file = fopen(FILE_NAME, "w+");

    if (!file)
    {
        printf("Error opening file!");
        exit(1);
    }

    pthread_mutex_lock(&fileLock);

    for (int i = 0; i < num_values; i++)
    {
        fprintf(file, "%d\t", getRandomNum(0, maxValue));
    }

    pthread_mutex_unlock(&fileLock);

    fclose(file);
}

/* FILE FUNCTIONS */

/**
 * Parse entire file
 *
 * This function is fairly redundant compared to
 * parseSubFile but oh well
 *
 * @param nums - int[] to store file contents in
 */
void parseFile(int *nums)
{
    pthread_mutex_lock(&fileLock);

    FILE *file = fopen(FILE_NAME, "r");

    fseek(file, 0L, SEEK_END);
    int size = ftell(file) + 1;
    rewind(file);

    char fileContent[size];

    fgets(fileContent, size, file);

    pthread_mutex_unlock(&fileLock);

    char *tmp;
    char *fileContents = fileContent;

    tmp = strtok_r(fileContents, "\t", &fileContents);

    for (int i = 0; i < num_values; i++)
    {
        nums[i] = atoi(tmp);
        tmp = strtok_r(fileContents, "\t", &fileContents);
    }
}

/**
 * Prints the file contents to terminal
 */
void printFile()
{
    int allNums[num_values];

    FILE *file = fopen(FILE_NAME, "r");

    if (!file)
    {
        printf("Error opening file!");
        return;
    }

    parseFile(allNums);

    printf("File Contents:\n[");
    for (int i = 0; i < num_values - 1; i++)
    {
        printf("%d, ", allNums[i]);
    }
    printf("%d]\n", allNums[num_values - 2]);

    fclose(file);
}

/**
 * Gets the byte position of an index in the file
 *
 * @param index - index of value
 * @param flag - 0 for start 1 for end
 * @return
 */
long int getFilePosition(int index, int flag)
{
    FILE *file = fopen(FILE_NAME, "r");
    long int startPos = 0;
    long int endPos = 0;
    char c;

    while (endPos != index && (c = fgetc(file)) != -1)
    {
        if (c == '\t' || c == '\0')
        {
            endPos++;
            startPos = ftell(file);
        }
    }

    c = fgetc(file);
    while (c != '\t')
    {
        c = fgetc(file);
    }
    endPos = ftell(file);

    fclose(file);

    if (flag == 1)
        return endPos;
    else
        return startPos;
}

/**
 * Retrieves sub sequence from file
 *
 * Uses Mutex lock on file to avoid conflicts
 *
 * @param start - i
 * @param end - j
 * @param nums - Array to store range of numbers
 * @param size - size of range
 */
void parseSubFile(int start, int end, int *nums, int size, int tid)
{
    pthread_mutex_lock(&fileLock);

    FILE *file = fopen(FILE_NAME, "r");
    int bufferSize;

    long int startPosition = getFilePosition(start, 0);
    long int endPosition = getFilePosition(end, 1);

    bufferSize = (endPosition - startPosition) + 1;

    char fileContent[bufferSize];

    fseek(file, startPosition, SEEK_SET);
    fgets(fileContent, bufferSize, file);

    fclose(file);
    pthread_mutex_unlock(&fileLock);

    char *tmp;
    char *fileContents = fileContent;

    tmp = strtok_r(fileContents, "\t", &fileContents);

    for (int i = 0; i < size; i++)
    {
        nums[i] = atoi(tmp);
        tmp = strtok_r(fileContents, "\t", &fileContents);
    }
}

/* SORTING FUNCTIONS */

/**
 * Sorts a randomly decided subsequence of the file
 *
 * @param struct thread_args -
 *  args.tid = thread id
 *  args.start = i
 *  args.end = j
 *  args.numEls = number of elements in range
 */
void sortSubFile(struct thread_args *args)
{
    int numEls = args->numEls;
    int start = args->start;
    int end = args->end;

    int nums[numEls];

    parseSubFile(start, end, nums, numEls, args->tid);

    /* Pick a random sorting algorithm */

    switch (getRandomNum(0, 2))
    {
    case 0:
        printf("Thread #%d grabbing the index range %d to %d\n", args->tid, args->start, args->end);
        printf("Using QuickSort...\n");
        quickSort(nums, 0, numEls - 1);
        break;
    case 1:
        printf("Thread #%d grabbing the index range %d to %d\n", args->tid, args->start, args->end);
        printf("Using BubbleSort...\n");
        bubbleSort(nums, numEls);
        break;
    case 2:
        printf("Thread #%d grabbing the index range %d to %d\n", args->tid, args->start, args->end);
        printf("Using Insertion Sort...\n");
        insertionSort(nums, numEls);
        break;
    }

    printf("Thread #%d done sorting...\n\n", args->tid);

    pthread_mutex_lock(&fileLock);

    long int startPos = getFilePosition(start, 0);

    FILE *file = fopen(FILE_NAME, "r+");
    fseek(file, startPos, SEEK_SET);

    for (int i = 0; i < numEls; i++)
    {
        fprintf(file, "%d\t", nums[i]);
    }

    fclose(file);

    pthread_mutex_unlock(&fileLock);
}

/**
 * Checks if the file is sorted
 *
 * @return - 0 false | 1 true
 */
void checkIfSorted()
{
    int allNums[num_values];
    parseFile(allNums);

    for (int i = 1; i < num_values; i++)
    {
        if (allNums[i - 1] > allNums[i])
            return;
    }
    printf("The File is Sorted!\n\n");
    fileIsSorted = 1;
}

/**
 * check if there is a "Lock" applied to range
 * @param start - i
 * @param end - j
 * @return - 0 unavailable | 1 available
 */
int isConflict(int start, int end)
{
    for (int i = start; i <= end; i++)
    {
        if (iLOCKED[i] == 1)
            return 0;
    }

    return 1;
}

/* THREAD FUNCTIONS */

/**
 * Manages the threads while they work to sort the file
 *
 * @param i - tid
 * @return
 */
void *sortFile(void *i)
{
    int tid = (int)i;

    while (fileIsSorted == 0)
    {
        /* Randomly generate i j pairs */
        int start = getRandomNum(0, num_values - 1);
        int end = getRandomNum(start, num_values - 1);
        int numEls = (end - start) + 1;

        /* Attempt to "Lock" portion of file */
        pthread_mutex_lock(&sortingLock);

        if (isConflict(start, end) == 0)
        {
            pthread_mutex_unlock(&sortingLock);
            continue;
        }

        /* "Lock" portion of file */

        for (int i = start; i <= end; i++)
        {
            iLOCKED[i] = 1;
        }

        pthread_mutex_unlock(&sortingLock);

        struct thread_args args;
        args.numEls = numEls;
        args.start = start;
        args.end = end;
        args.tid = tid;

        sortSubFile(&args);

        /* "Unlock" portion of file */
        for (int i = start; i <= end; i++)
        {
            iLOCKED[i] = 0;
        }
    }

    pthread_exit(NULL);
}

/**
 * watcher thread
 * every .1 sec, checks if the file is sorted
 *
 * @param i
 * @return
 */
void *monitorFile(void *i)
{
    while (fileIsSorted == 0)
    {
        printf("*** Watcher Thread is checking the file! ***\n\n");
        checkIfSorted();
        usleep(10000);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    num_values = parseCLI(argc, argv);
    if (num_values == 0)
        return 1;

    /* Discount version of record locks */
    iLOCKED = malloc(sizeof(int) * num_values);
    jLOCKED = malloc(sizeof(int) * num_values);
    memset(iLOCKED, 0, sizeof(&iLOCKED));
    memset(jLOCKED, 0, sizeof(&jLOCKED));

    srand(time(0));

    //int m = NUM_THREADS;
    int m = getUserInput(num_values);

    populateFile(100); /* maxValue sets size of numbers generated (inclusive) */

    printFile();

    pthread_mutex_init(&sortingLock, NULL);
    pthread_mutex_init(&fileLock, NULL);

    pthread_t watcher;
    pthread_t threads[m];

    /* CREATE THREADS */

    pthread_create(&watcher, NULL, monitorFile, NULL);

    for (int i = 0; i < m; i++)
        pthread_create(&threads[i], NULL, sortFile, (void *)(long)i);

    /* JOIN THREADS */

    pthread_join(watcher, NULL);

    for (int i = 0; i < m; i++)
        pthread_join(threads[i], NULL);

    printFile();

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <math.h>
#include <string.h>
#include <pthread.h>

#include "linkedList.h"

#include <time.h>
#include <sys/time.h>
#define BILLION 1E9

#define INSIZE 10

char fileBuffer[INSIZE];

int N = 5; // Default value of N

struct Node **headArray;

struct threadArguements
{
    int t_index;
};

FILE *openIntermediateFile(int fileNumber, char *mode)
{
    char strBuffer[64];
    sprintf(strBuffer, "file_%d.txt", fileNumber);
    return fopen(strBuffer, mode);
}

int checkPrime(int number)
{
    if (number == 1)
        return 0;

    int squareRoot = sqrt(number);
    int divider;
    for (divider = 2; divider <= squareRoot; divider++)
    {
        if (number % divider == 0)
        {
            return 0;
        }
    }

    return 1;
}

// Function to display program usage
void display_usage()
{
    printf("Usage: ./primeT -n <N> -i <input_file> -o <output_file>\n");
}

static void *threadFunction(void *ptrThreadStruct)
{
    struct threadArguements *tmpArgs = ptrThreadStruct;

    // Open intermediate files in read mode
    FILE *intermediateFile = openIntermediateFile(tmpArgs->t_index + 1, "r");
    struct Node **head = &headArray[tmpArgs->t_index];

    char resultRead[INSIZE]; // local buffer
    while (1) {
        char *result = fgets(resultRead, INSIZE, intermediateFile);

        if (result == NULL) {
            break; // End of file
        }

        int candidateNumber = atoi(resultRead);
        int isPrime = checkPrime(candidateNumber);
        if (isPrime)
        {
            insert(head, candidateNumber);
        }
    }

    // Close intermediate file
    fclose(intermediateFile);

    // Terminate thread
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    // Start Clock
    struct timespec startTime, stopTime;
    clock_gettime(CLOCK_REALTIME, &startTime);

    char *inputFileName = NULL;
    char *outputFileName = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "n:i:o:")) != -1)
    {
        switch (opt)
        {
        case 'n':
            N = atoi(optarg);

            if (N < 1 || N > 20)
            {
                fprintf(stderr, "Error: Minimum and maximum values for N are 1 and 20.\n");
                exit(EXIT_FAILURE);
            }

            break;
        case 'i':
            inputFileName = optarg;
            break;
        case 'o':
            outputFileName = optarg;
            break;
        default:
            display_usage();
            exit(EXIT_FAILURE);
        }
    }

    // Check if required options are provided
    if (inputFileName == NULL || outputFileName == NULL)
    {
        fprintf(stderr, "Error: Input and output files are required.\n");
        display_usage();
        exit(EXIT_FAILURE);
    }

    // Open the input file in read mode
    FILE *inputFile;
    inputFile = fopen(inputFileName, "r");

    // Check if the input file is opened successfully
    if (inputFile == NULL)
    {
        printf("Error in opening input file\n");
        exit(1);
    }

    // Open the intermediate files in write mode and check if they opened successfully
    FILE *intermediateFiles[N];
    for (int i = 0; i < N; ++i)
    {
        intermediateFiles[i] = openIntermediateFile(i + 1, "w");

        if (intermediateFiles[i] == NULL)
        {
            printf("Error in opening intermediate file %d\n", i + 1);
            exit(1);
        }
    }

    // Distribute the number in the input file into intermediate files
    char *fileBufferPointer;
    int fileCounter = 1;

    while ((fileBufferPointer = fgets(fileBuffer, INSIZE, inputFile)))
    {
        int resultWrite = fputs(fileBufferPointer, intermediateFiles[(fileCounter - 1) % N]);

        if (resultWrite < 0)
        {
            printf("Error while writing to intermediate files\n");
            break;
        }
        ++fileCounter;
    }

    // Close input file
    fclose(inputFile);

    // Close intermediate files
    for (int i = 0; i < N; ++i)
    {
        fclose(intermediateFiles[i]);
    }

    // Allocate thread id and thread arguments arrays
    pthread_t threadIds[N];
    struct threadArguements threadArgsArray[N];

    // Allocate pointers
    headArray = (struct Node **)malloc(sizeof(struct Node *) * N);
    for (int i = 0; i < N; i++)
    {
        headArray[i] = NULL;
    }

    // Create threads
    int threadCounter;
    int threadResult;
    for (threadCounter = 0; threadCounter < N; ++threadCounter)
    {
        threadArgsArray[threadCounter].t_index = threadCounter;

        threadResult = pthread_create(&(threadIds[threadCounter]),
                                      NULL, threadFunction, (void *)&(threadArgsArray[threadCounter]));

        if (threadResult != 0)
        {
            printf("Thread creation failed \n");
            exit(1);
        }
    }

    // Wait for all threads to terminate
    for (threadCounter = 0; threadCounter < N; ++threadCounter)
    {
        threadResult = pthread_join(threadIds[threadCounter], NULL);
        if (threadResult != 0)
        {
            printf("Thread join failed\n");
            exit(1);
        }
    }

    // Create output file to write all prime numbers
    FILE *outputFile = fopen(outputFileName, "w");

    // Write the data of all linked lists to output file
    for (int i = 0; i < N; i++)
    {
        struct Node *ptrNode = headArray[i];
        while (ptrNode != NULL)
        {
            char strBuffer[64];
            sprintf(strBuffer, "%d\n", ptrNode->item);

            int resultWrite = fputs(strBuffer, outputFile);

            if (resultWrite < 0)
            {
                printf("Error while writing to intermediate files\n");
                break;
            }

            ptrNode = ptrNode->next;
        }
    }

    // Close output file
    fclose(outputFile);

    // Close and remove intermediate files
    for (int threadCounter = 0; threadCounter < N; ++threadCounter)
    {
        char intermediateFileName[64];
        sprintf(intermediateFileName, "file_%d.txt", threadCounter + 1);

        if (remove(intermediateFileName) != 0)
        {
            perror("Error removing intermediate file\n");
            exit(1);
        }
    }

    // Clean up
    for (int i = 0; i < N; i++)
    {
        freeList(headArray[i]);
    }

    free(headArray);

    // End Clock
    clock_gettime(CLOCK_REALTIME, &stopTime);
    double timeElapsed = (stopTime.tv_sec - startTime.tv_sec) +
                         (stopTime.tv_nsec - startTime.tv_nsec) / BILLION;
    printf("%lf\n", timeElapsed);
}

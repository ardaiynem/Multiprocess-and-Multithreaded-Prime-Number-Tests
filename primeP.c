#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>

#include <mqueue.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <math.h>

#include <string.h>

#define MQNAME "/ipc_prime"
#define INSIZE 10

#include <time.h>
#include <sys/time.h>
#define BILLION 1E9

char fileBuffer[INSIZE];

int N = 5; // Default value for N
int M = 3; // Default value for M

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
    printf("Usage: ./primeP -n <N> -m <M> -i <input_file> -o <output_file>\n");
}

int main(int argc, char *argv[])
{
    // Start Clock
    struct timespec startTime, stopTime;
    clock_gettime(CLOCK_REALTIME, &startTime);

    char *inputFileName = NULL;
    char *outputFileName = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "n:m:i:o:")) != -1)
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
        case 'm':
            M = atoi(optarg);

            if (M < 1 || M > 20)
            {
                fprintf(stderr, "Error: Minimum and maximum values for M are 1 and 20.\n");
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

    // Create struct with the input value or default value of M
    struct messageStruct
    {
        int messageStructLength;
        int values[M];
    };

    // Declare message queue, item, buffer and buffer length
    mqd_t messageQueue;
    struct messageStruct *messageItem;
    char *messageBuffer;
    int messageBufferLength;

    // Open message queue in read & write modes with 0666 permission and decided attributes
    struct mq_attr mqAttributes;
    mqAttributes.mq_maxmsg = 10;  // Maximum number of messages in the queue
    mqAttributes.mq_msgsize = 128; // Maximum size of each message (Enough to hold max number of M number 84 bytes)
    messageQueue = mq_open(MQNAME, O_RDWR | O_CREAT, 0666, &mqAttributes);

    // Check if message queue is opened successfully
    if (messageQueue == -1)
    {
        perror("Can't create message queue!\n");
        exit(1);
    }

    // Allocate buffer lenghts from the corresponding message queue attributes
    messageBufferLength = mqAttributes.mq_msgsize;
    messageBuffer = (char *)malloc(messageBufferLength);

    // Create N child processes
    pid_t pid;

    for (int i = 0; i < N; ++i)
    {
        // Create a child process
        pid = fork();

        // Check if creating a child failed
        if (pid < 0)
        {
            printf("fork() failed!\n");
            exit(1);
        }
        if (pid == 0)
        {
            // Open intermediate files in read mode
            FILE *intermediateFile = openIntermediateFile(i + 1, "r");

            char *resultRead;
            int primeCounter = 0;
            int primeBuffer[M];
            while ((resultRead = fgets(fileBuffer, INSIZE, intermediateFile)))
            {
                int candidateNumber = atoi(resultRead);
                int isPrime = checkPrime(candidateNumber);

                if (isPrime)
                {
                    primeBuffer[primeCounter++] = candidateNumber;

                    if (primeCounter != 0 && (primeCounter % M) == 0)
                    {
                        messageItem = (struct messageStruct *)messageBuffer;
                        messageItem->messageStructLength = M;

                        for (int i = 0; i < M; i++)
                        {
                            (messageItem->values)[i] = primeBuffer[i];
                        }

                        int msgResult;
                        msgResult = mq_send(messageQueue, messageBuffer, messageBufferLength, 0);
                        if (msgResult == -1)
                        {
                            perror("mq_send() failed!\n");
                            exit(1);
                        }

                        primeCounter = 0;
                    }
                }
            }

            // Send remaining primes
            if (primeCounter != 0)
            {
                messageItem = (struct messageStruct *)messageBuffer;
                messageItem->messageStructLength = primeCounter;

                for (int i = 0; i < primeCounter; i++)
                {
                    (messageItem->values)[i] = primeBuffer[i];
                }

                int msgResult;
                msgResult = mq_send(messageQueue, messageBuffer, messageBufferLength, 0);
                if (msgResult == -1)
                {
                    perror("mq_send() failed!\n");
                    exit(1);
                }
            }

            // Close intermediate file
            fclose(intermediateFile);

            // Terminate child
            exit(0);
        }
    }

    // Create output file to write all prime numbers
    FILE *outputFile = fopen(outputFileName, "w");

    // Track the amount of active children
    int active_children = N;
    while (active_children > 0)
    {
        // Time specification for mq_timedreceive()
        struct timespec timeout;
        timeout.tv_sec = 0;
        timeout.tv_nsec = 10000000; // 10 milliseconds

        // Timed recieve
        int receivedResult = mq_timedreceive(messageQueue, messageBuffer, messageBufferLength, NULL, &timeout);

        if (receivedResult == -1)
        {
            // Check if error is related to timeout or recieve fail
            if (errno != ETIMEDOUT)
            {
                perror("mq_receive() failed!\n");
                exit(1);
            }
        }
        else
        {
            messageItem = (struct messageStruct *)messageBuffer;

            for (int i = 0; i < messageItem->messageStructLength; i++)
            {
                char strBuffer[64];
                sprintf(strBuffer, "%d\n", messageItem->values[i]);

                int resultWrite = fputs(strBuffer, outputFile);

                if (resultWrite < 0)
                {
                    printf("Error while writing to intermediate files\n");
                    break;
                }
            }
        }

        // Check for terminated child processes without blocking
        pid_t terminated_child;
        terminated_child = waitpid(-1, NULL, WNOHANG);
        if (terminated_child > 0)
        {
            active_children--;
        }
    }

    // Close output file
    fclose(outputFile);

    // Close and remove intermediate files
    for (int i = 0; i < N; ++i)
    {
        char intermediateFileName[64];
        sprintf(intermediateFileName, "file_%d.txt", i + 1);

        if (remove(intermediateFileName) != 0)
        {
            perror("Error removing intermediate file\n");
            exit(1);
        }
    }

    // Clean up
    free(messageBuffer);
    mq_close(messageQueue);
    mq_unlink(MQNAME);


    // End Clock
    clock_gettime(CLOCK_REALTIME, &stopTime);
    double timeElapsed = (stopTime.tv_sec - startTime.tv_sec) +
                         (stopTime.tv_nsec - startTime.tv_nsec) / BILLION;
    printf("%lf\n", timeElapsed);
}

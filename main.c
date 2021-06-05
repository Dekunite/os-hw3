/*
<Name Surname: Muhammet Derviş Kopuz>
<Student Number: 504201531>
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

//  for shared memory and semaphores
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>

//  for handling signals
#include <signal.h>

key_t READNEWS = 363;
key_t PUBLISHED = 364;
key_t KEYSHM = 365;
key_t BOOLSHM = 366;

//news source processes
int m = 3;
//subscriber processes
const int n = 5;

//news data
char d;
//assign memory size
int messageMemory = sizeof(d);
int arrayMemory = (sizeof(int)*n);

int shmid = 0;

int publishSem = 0;
int readNewsSem = 0;

//increase semaphore value
void sem_signal(int semid, int val)
{
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = val;  //  relative:  add sem_op to value
    semaphore.sem_flg = 1;   
    semop(semid, &semaphore, 1);
}

//decrease semaphore value
void sem_wait(int semid, int val)
{
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = (-1*val);  //  relative:  add sem_op to value
    semaphore.sem_flg = 1;  
    semop(semid, &semaphore, 1);
}

int semValue(int semid) {
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = 1;
    semaphore.sem_flg = 1;  
    semop(semid, &semaphore, 1);
    printf("sem val: %d \n", semaphore.sem_op);
}

void publish(char d) {
    //get PUBLISHED semaphore
    publishSem = semget(PUBLISHED, 1, 0);
    //get READNEWS semaphore
    readNewsSem = semget(READNEWS, 1, 0);
    //try to decrease published semaphore by 1
    //if it is already 0, previous news wasnt fetched by all subscribers
    sem_wait(publishSem, 1);

    //get shared memory
    shmid = shmget(KEYSHM, messageMemory, 0);
    int* messageMemoryPtr = (int*) shmat(shmid,0,0);

    //write message in memory
    *(messageMemoryPtr) = d;
    printf("published the message: %c \n", d);
    //set boolean array to all 0's
    int boolShmid = shmget(BOOLSHM, arrayMemory, 0);
    int* arrayMemoryPtr = (int*) shmat(boolShmid,0,0);
    for (int i = 0; i<n; i++) {
        *(arrayMemoryPtr + (sizeof(int) * i)) = 0;
    }
    //detach the array memory segment
    shmdt(arrayMemoryPtr);
    //set readnews sem to subscriber count
    sem_signal(readNewsSem, n);

    //detach the shared memory
    shmdt(messageMemoryPtr);

}

void read_news() {
    //get READNEWS semaphore
    readNewsSem = semget(READNEWS, 1, 0);

    //get message shared memory
    shmid = shmget(KEYSHM, messageMemory, 0);
    int* messageMemoryPtr = (int*) shmat(shmid,0,0);

    //read message message in memory
    char message = *(messageMemoryPtr);
    printf("received the message: %c \n", message);

    //detach the message memory segment
    shmdt(messageMemoryPtr);

    //decrease readnews sem by 1
    sem_wait(readNewsSem, 1);
    printf("readnesem: %d \n", readNewsSem);

    //if readnews == 0
    
}


int main(int argc, char* argv[]) {
    int* memoryPtr = NULL;

    //create shared memory with memory size / for message
    int messageShmid = shmget(KEYSHM, messageMemory, IPC_CREAT | 0700);
    //attach shared memory
    memoryPtr = (int *) shmat(messageShmid, 0, 0);
    //detach the shared memory segment
    shmdt(memoryPtr);

    int zero = 0;
    //create shared memory with subscriber numbers / for bool array (fetched)
    int arrayShmid = shmget(BOOLSHM, arrayMemory, IPC_CREAT | 0700);
    //attach shared memory
    memoryPtr = (int *) shmat(arrayShmid, 0, 0);
    //initialize bool array as 0
    for (int i = 0; i<n; i++) {
        *(memoryPtr + (sizeof(int) * i)) = zero;
    }
    //detach the shared memory segment
    shmdt(memoryPtr);

    //create PUBLISHED SEMAPHORE
    //initialized as 1 as there can only be 1 news 
    publishSem = semget(PUBLISHED, 1, 0700|IPC_CREAT);
    semctl(publishSem, 0, SETVAL, 1);
    printf("pubbb %d", publishSem);

    //create READNEWS semaphore
    //initialized as 0. The number of subscribers
    readNewsSem = semget(READNEWS, 1, 0700|IPC_CREAT);
    semctl(readNewsSem, 0, SETVAL, 0);
    printf("reaaaaaad %d \n", readNewsSem);

    publish('x');
    read_news();
    read_news();
    read_news();
    read_news();
    read_news();
    semValue(readNewsSem);
    semValue(readNewsSem);
    semValue(readNewsSem);
    //need to create processes with fork in loop

    int result;
    int i;
    //  create 2 child processes
    for (i = 0; i < 2; i++)
    {
        result = fork();
        if (result < 0)
        {
            printf("FORK error...\n");
            exit(1);
        }
        if (result == 0) {
            break;
        } else {
            printf("number:  %d \n", result);
            //publish('y');
            //read_news();
        }
    }


    return 0;
}

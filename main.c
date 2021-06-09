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

#include <pthread.h>

key_t READNEWS = 363;
key_t PUBLISHED = 364;
key_t KEYSHM = 365;
key_t FETCHED = 367;
key_t READ0 = 368;
key_t READ1 = 369;
key_t READ2 = 370;
key_t READ3 = 371;
key_t READ4 = 372;

//news source processes
int m = 3;
//subscriber processes
const int n = 5;

//news data
char d;
//assign memory size
int messageMemory = sizeof(d);

int fetched[5];
int readerSemaphores[5];
int publishCount = 0;

int shmid = 0;

int publishSem = 0;
int fetchedSem = 0;

int readerSem0 = 0;
int readerSem1 = 0;
int readerSem2 = 0;
int readerSem3 = 0;
int readerSem4 = 0;

pthread_t pub1, pub2, pub3,pub4, pub5, pub6, read1,read2,read3,read4,read5;  //  create threads 

void* reader();

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

void publish(char d) {
    //get PUBLISHED semaphore
    publishSem = semget(PUBLISHED, 1, 0);
    //get fetched semaphore
    fetchedSem = semget(FETCHED, 1, 0);

    //get reader semaphores
    readerSem0 = semget(READ0, 1, 0);
    readerSem1 = semget(READ1, 1, 0);
    readerSem2 = semget(READ2, 1, 0);
    readerSem3 = semget(READ3, 1, 0);
    readerSem4 = semget(READ4, 1, 0);
    //try to decrease published semaphore by 1
    //if it is already 0, previous news wasnt fetched by all subscribers
    sem_wait(publishSem, 1);

    //increase publish count with every new message published
    publishCount += 1;

    //get shared memory
    shmid = shmget(KEYSHM, messageMemory, 0);
    int* messageMemoryPtr = (int*) shmat(shmid,0,0);

    //write message in memory
    *(messageMemoryPtr) = d;
    printf("Publisher %d published the message: %c \n",publishCount, d);

    //go into critical section for fetched array
    sem_wait(fetchedSem,1);
    //set boolean array to all 0's
    int i = 0;
    for(i = 0; i < n; i++) {
        fetched[i] = 0;
    }
    //leave critical section for fetched array
    sem_signal(fetchedSem,1);

    //detach the shared memory
    shmdt(messageMemoryPtr);

    //signal every reader semaphore since there is a new message
    sem_signal(readerSem0,1);
    sem_signal(readerSem1,1);
    sem_signal(readerSem2,1);
    sem_signal(readerSem3,1);
    sem_signal(readerSem4,1);

}

void read_news(int num) {
    //get PUBLISHED semaphore
    publishSem = semget(PUBLISHED, 1, 0);
    //get fetched semaphore
    fetchedSem = semget(FETCHED, 1, 0);

    //get message shared memory
    shmid = shmget(KEYSHM, messageMemory, 0);
    int* messageMemoryPtr = (int*) shmat(shmid,0,0);

    //read message message in memory
    char message = *(messageMemoryPtr);
    printf("Subscriber %d fetched the message: %c \n",num, message);

    //detach the message memory segment
    shmdt(messageMemoryPtr);

    //check if every subscriber fetched the news
    int allRead = 1;
    int i = 0;
    for(i = 0; i<n; i++) {
        if(fetched[i] == 0) {
            //not every sub received the news
            allRead = 0;
        }
    }
    if(allRead == 1) {
        //new news can be published
        sem_signal(publishSem,1);
    }
    
}

void* publisher(void* message) {
    char mes = (intptr_t) message;
    publish(mes);
    pthread_exit(NULL);
}

void* reader(void* number) {
    //get fetched semaphore
    fetchedSem = semget(FETCHED, 1, 0);
    //get reader semaphores
    readerSem0 = semget(READ0, 1, 0);
    readerSem1 = semget(READ1, 1, 0);
    readerSem2 = semget(READ2, 1, 0);
    readerSem3 = semget(READ3, 1, 0);
    readerSem4 = semget(READ4, 1, 0);

    while (1) {

        int num = ((intptr_t) number);
        //mark specific reader semaphore as fetched
        //wait if there is no new message / SUSPEND
        sem_wait(readerSemaphores[num],1);
        //critical section for fetched array
        sem_wait(fetchedSem,1);
        fetched[num] = 1;
        read_news(num);
        //leave critical section for fetched array
        sem_signal(fetchedSem,1);

        //exit subscriber thread if published messages equals to publisher count
        if(publishCount == m) {
            pthread_exit(NULL);
        }
    }
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
    //initialize bool array as 0
    int i = 0;
    for(i = 0; i < n; i++) {
        fetched[i] = zero;
    }

    //create PUBLISHED SEMAPHORE
    //initialized as 1 as there can only be 1 news 
    publishSem = semget(PUBLISHED, 1, 0700|IPC_CREAT);
    semctl(publishSem, 0, SETVAL, 1);

    //create fetched SEMAPHORE
    //initialized as 1 as there can only be 1 news 
    fetchedSem = semget(FETCHED, 1, 0700|IPC_CREAT);
    semctl(fetchedSem, 0, SETVAL, 1);

    //create readers semaphores
    readerSem0 = semget(READ0, 1, 0700|IPC_CREAT);
    semctl(readerSem0, 0, SETVAL, 0);

    readerSem1 = semget(READ1, 1, 0700|IPC_CREAT);
    semctl(readerSem1, 0, SETVAL, 0);

    readerSem2 = semget(READ2, 1, 0700|IPC_CREAT);
    semctl(readerSem2, 0, SETVAL, 0);

    readerSem3 = semget(READ3, 1, 0700|IPC_CREAT);
    semctl(readerSem3, 0, SETVAL, 0);

    readerSem4 = semget(READ4, 1, 0700|IPC_CREAT);
    semctl(readerSem4, 0, SETVAL, 0);

    //push readers semaphores in to a list
    readerSemaphores[0] = readerSem0;
    readerSemaphores[1] = readerSem1;
    readerSemaphores[2] = readerSem2;
    readerSemaphores[3] = readerSem3;
    readerSemaphores[4] = readerSem4;

    //create 3 publisher processes with different messages
    pthread_create(&pub1, NULL, publisher, (void*)(intptr_t)'x');
    pthread_create(&pub2, NULL, publisher, (void*)(intptr_t)'y');
    pthread_create(&pub3, NULL, publisher, (void*)(intptr_t)'z');

    //create 5 subscriber processes
    pthread_create(&read1, NULL, reader, (void*)(intptr_t) 0);
    pthread_create(&read2, NULL, reader, (void*)(intptr_t) 1);
    pthread_create(&read3, NULL, reader, (void*)(intptr_t) 2);
    pthread_create(&read4, NULL, reader, (void*)(intptr_t) 3);
    pthread_create(&read5, NULL, reader, (void*)(intptr_t) 4);

    //  wait for threads to finish
    pthread_join(pub1, NULL);
    pthread_join(pub2, NULL);
    pthread_join(pub3, NULL);
    pthread_join(read1, NULL);
    pthread_join(read2, NULL);
    pthread_join(read3, NULL);
    pthread_join(read4, NULL);
    pthread_join(read5, NULL);

    //remove all semaphores
    semctl(publishSem, 0, IPC_RMID, 0);
    semctl(fetchedSem, 0, IPC_RMID, 0);
    semctl(readerSem0, 0, IPC_RMID, 0);
    semctl(readerSem1, 0, IPC_RMID, 0);
    semctl(readerSem2, 0, IPC_RMID, 0);
    semctl(readerSem3, 0, IPC_RMID, 0);
    semctl(readerSem4, 0, IPC_RMID, 0);

    return 0;
}

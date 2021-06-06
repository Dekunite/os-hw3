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

#include <pthread.h>

key_t READNEWS = 363;
key_t PUBLISHED = 364;
key_t KEYSHM = 365;
key_t BOOLSHM = 366;
key_t FETCHED = 367;

//news source processes
int m = 3;
//subscriber processes
const int n = 5;

//news data
char d;
//assign memory size
int messageMemory = sizeof(d);
int arrayMemory = (sizeof(int)*n);

int fetched[5];

int shmid = 0;

int publishSem = 0;
int readNewsSem = 0;
int fetchedSem = 0;

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
    //get fetched semaphore
    fetchedSem = semget(FETCHED, 1, 0);
    //try to decrease published semaphore by 1
    //if it is already 0, previous news wasnt fetched by all subscribers
    sem_wait(publishSem, 1);

    //get shared memory
    shmid = shmget(KEYSHM, messageMemory, 0);
    int* messageMemoryPtr = (int*) shmat(shmid,0,0);

    //write message in memory
    *(messageMemoryPtr) = d;
    printf("published the message: %c \n", d);

    sem_wait(fetchedSem,1);
    //set boolean array to all 0's
    for(int i = 0; i < n; i++) {
        fetched[i] = 0;
    }
    sem_signal(fetchedSem,1);
    //int boolShmid = shmget(BOOLSHM, arrayMemory, 0);
    //int* arrayMemoryPtr = (int*) shmat(boolShmid,0,0);
    //for (int i = 0; i<n; i++) {
      //  *(arrayMemoryPtr + (sizeof(int) * i)) = 0;
    //}
    //detach the array memory segment
    //shmdt(arrayMemoryPtr);
    //set readnews sem to subscriber count
    sem_signal(readNewsSem, n);

    //detach the shared memory
    shmdt(messageMemoryPtr);

}

void read_news() {
    //get READNEWS semaphore
    readNewsSem = semget(READNEWS, 1, 0);
    //get PUBLISHED semaphore
    publishSem = semget(PUBLISHED, 1, 0);
    //get fetched semaphore
    fetchedSem = semget(FETCHED, 1, 0);

    //get message shared memory
    shmid = shmget(KEYSHM, messageMemory, 0);
    int* messageMemoryPtr = (int*) shmat(shmid,0,0);

    //read message message in memory
    char message = *(messageMemoryPtr);
    printf("received the message: %c \n", message);

    //detach the message memory segment
    shmdt(messageMemoryPtr);

    //decrease readnews sem by 1
    //sem_wait(readNewsSem, 1);

    //if readnews == 0
    //increase publish sem when all subs read the news
    //if fetched[] all 1's
    sem_wait(fetchedSem,1);
    int allRead = 1;
    for( int i = 0; i<n; i++) {
        if(fetched[i] == 0) {
            //not every sub received the news
            allRead = 0;
        }
    }
    if(allRead == 1) {
        //new news can be published
        sem_signal(publishSem,1);
    }
    sem_signal(fetchedSem,1);
    
}

void* publisher(void* message) {
    char mes = (char) message;
    publish(mes);
}

void* reader(void* number) {
    while (1) {
        int num = (int) number;
        //get fetched semaphore
        fetchedSem = semget(FETCHED, 1, 0);
        if(fetched[num] == 0) {
            fetched[num] = 1;
            read_news();
            //set fetched[num] as read/1
        } else {
            //suspend thread
            printf("No new message for subscriber %d \n", num);
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
    //int arrayShmid = shmget(BOOLSHM, arrayMemory, IPC_CREAT | 0700);
    //attach shared memory
    //memoryPtr = (int *) shmat(arrayShmid, 0, 0);
    //initialize bool array as 0
    for(int i = 0; i < n; i++) {
        fetched[i] = zero;
    }
    //for (int i = 0; i<n; i++) {
      //  *(memoryPtr + (sizeof(int) * i)) = zero;
    //}
    //detach the shared memory segment
    //shmdt(memoryPtr);

    //create PUBLISHED SEMAPHORE
    //initialized as 1 as there can only be 1 news 
    publishSem = semget(PUBLISHED, 1, 0700|IPC_CREAT);
    semctl(publishSem, 0, SETVAL, 1);

    //create READNEWS semaphore
    //initialized as 0. The number of subscribers
    readNewsSem = semget(READNEWS, 1, 0700|IPC_CREAT);
    semctl(readNewsSem, 0, SETVAL, 0);

    //create fetched SEMAPHORE
    //initialized as 1 as there can only be 1 news 
    fetchedSem = semget(FETCHED, 1, 0700|IPC_CREAT);
    semctl(fetchedSem, 0, SETVAL, 1);

    //publish('x');
    //read_news();
    //read_news();
    //read_news();
    //read_news();
    //read_news();
    //semValue(readNewsSem);
    //semValue(readNewsSem);
    //semValue(readNewsSem);
    //need to create processes with fork in loop

    pthread_t pub1, pub2, pub3, read1,read2,read3,read4,read5;  //  create threads 
    pthread_create(&pub1, NULL, publisher, (void*)'u');
    pthread_create(&pub2, NULL, publisher, (void*)'o');
    pthread_create(&pub3, NULL, publisher, (void*)'k');
    //for (int k = 0; k<n; k++) {
      //  pthread_create(&read1, NULL, reader, (void*) k);
    //}
    pthread_create(&read1, NULL, reader, (void*) 0);
    pthread_create(&read2, NULL, reader, (void*) 1);
    pthread_create(&read3, NULL, reader, (void*) 2);
    pthread_create(&read4, NULL, reader, (void*) 3);
    pthread_create(&read5, NULL, reader, (void*) 4);

    //  wait for threads to finish
    pthread_join(pub1, NULL);
    pthread_join(pub2, NULL);
    pthread_join(pub3, NULL);
    pthread_join(read1, NULL);
    pthread_join(read2, NULL);
    pthread_join(read3, NULL);
    pthread_join(read4, NULL);
    pthread_join(read5, NULL);

    return 0;
}

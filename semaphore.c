/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.

#include "err_exit.h"
#include "semaphore.h"

/*
int id;

id = semget(IPC_PRIVATE, 10 ,IPC_CREAT | S_IRUSR | S_IWUSR); //10 = quantità

//IPC_EXCL fa fallire semget se il set di semafori già esiste

if (id == -1){
    errExit(semget);
}

//dare i valori ai semafori


int semid = semget(key, 10, IPC_CREAT | S_IRUSR | S_IWUSR);
// set the first 5 semaphores to 1, and the remaining to 0
int values[] = {1,1,1,1,1,0,0,0,0,0};
union semun arg;
arg.array = values;
// initialize the semaphore set
if (semctl(semid, 0//valore nullo, SETALL, arg) == -1)
    errExit("semctl SETALL");
*/

void semOp (int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};

    if (semop(semid, &sop, 1) == -1)
        errExit("semop failed");
}
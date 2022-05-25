/// @file sender_manager.c
/// @brief Contiene l'implementazione del sender_manager.

#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"
#include <fcntl.h>

int main(int argc, char * argv[]) {

    
    int shmid = alloc_shared_memory(shmkey, 4096);
    
    int mesgid = msgget(msgkey , IPC_CREAT | S_IRUSR | S_IWUSR);//coda di messaggi

    if(mesgid == -1) 
        errExit("message queue non creata");
    
    if(mkfifo(pathnameFIFO1 , S_IRUSR | S_IWUSR) == -1)  //fifo 1
        errExit("FIFO1 non creata");
    
    if(mkfifo(pathnameFIFO2 ,  S_IRUSR | S_IWUSR) == -1) //fifo 2
        errExit("FIFO2 non creata");
   
    int FIFO1id = open(pathnameFIFO1 , O_RDONLY);

    if(FIFO1id == -1)
        errExit("errore apertura FIFO1");
   
    int n; // variabile buffer del numero dei file che abbiamo letto
   
    int fifo = read(FIFO1id , n , sizeof(n));
   
    if(fifo == -1 ) 
        errExit("errore nella lettura della FIFO1");
 
 
    int semid = semget(semkey, 2, S_IRUSR | S_IWUSR);   //set di 1 semaforo settato a 0, bloccante anche per il client
    semctl(semid, 0 , SETVAL, 0 );
    semctl(semid, 1, SETVAL, 1);
   
    char *ptr_shm= (char *)get_shared_memory(shmid , 0);  
    ptr_shm = "numero file ricevuto";
  
    semOp(semid,0,1);
   
    if (semctl(semid, 0 /*ignored*/, IPC_RMID, NULL) == -1)
        errExit("semctl IPC_RMID failed");
  
    return 0;
}

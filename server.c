/// @file sender_manager.c
/// @brief Contiene l'implementazione del sender_manager.

#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"
#include <fcntl.h>

#define  semkey 10 // da settare uguale al serben
#define  shmkey 20 // shared memori var globale
#define  msgKey 30 

#define pathnameFIFO1 "fifo1"
#define pathnameFIFO2 "fifo2"

//id globali
int FIFO1id , FIFO2id , shmid , mesgid ,  semid;

message_t* ptr_shm;

void quit(int sig){
   
    if (shmid != 0 ){
        free_shared_memory(ptr_shm);
        remove_shared_memory(shmid);
    }
   
    if(mesgid != 0 && (msgctl(mesgid, IPC_RMID, NULL)==-1) )
        ErrExit("message_queue non è stata chiusa\n");
   
    if (FIFO1id != 0 && close(FIFO1id) == -1)
        ErrExit("close failed\n");

    if (FIFO2id != 0 && close(FIFO2id) == -1)
        ErrExit("close failed\n");
  
    // Remove the FIFO
    if (unlink(pathnameFIFO1) != 0)
        ErrExit("unlink failed\n");
        
    if (unlink(pathnameFIFO2) != 0 )
        ErrExit("unlink failed\n");
    
    if(semid != 0 && (semctl(semid , 0 , IPC_RMID, NULL )== -1))
       ErrExit("set di semafori non eliminato\n");
       
    // terminate the process
    exit(0);
}

int main(int argc, char * argv[]) {

    sigset_t mySet;
  
    if(sigfillset(&mySet) == -1) {
        ErrExit("sigfillset fallito\n");
    }

    // Rimuove dall'insieme il segnale SIGINT
    if(sigdelset(&mySet, SIGINT) == -1) {
        ErrExit("sigdelset fallito\n");
    }

    // Blocca tutti i segnali eccetto SIGINT
    if(sigprocmask(SIG_SETMASK, &mySet, NULL) == -1) {
        ErrExit("sigprocmask fallito\n");
    }

    // all'arrivo di SIGINT devo togliere tutte le IPC aperte 
    if (signal(SIGINT, quit) == SIG_ERR)
        ErrExit("change signal handlers failed\n"); 

    //fine di creazione e cambio della maschera

    //creazione shared_memory , message_queue , FIFO1 , FIFO2 
    int shmid = alloc_shared_memory(shmkey, 4096);
    ptr_shm= (message_t*)get_shared_memory(shmid , 0); 
    
    int mesgid = msgget(msgKey , IPC_CREAT | S_IRUSR | S_IWUSR);//coda di messaggi

    if(mesgid == -1) 
        ErrExit("message queue non creata\n");
    
    if(mkfifo(pathnameFIFO1 , S_IRUSR | S_IWUSR | S_IWGRP) == -1)  //fifo 1
        ErrExit("FIFO1 non creata\n");
    
    if(mkfifo(pathnameFIFO2 ,  S_IRUSR | S_IWUSR | S_IWGRP ) == -1) //fifo 2
        ErrExit("FIFO2 non creata\n");
    
    //creo set di semafori come prima
    int semid = semget(semkey, 6, IPC_CREAT|S_IRUSR | S_IWUSR);  
    if(semid == -1)
        ErrExit("errore creazione semafori\n");
  
    union semun arg;
    unsigned short values[] = {0,0,50,50,50,50}; // inizializzazione del set di semafori
    arg.array = values;

    //setta i semafori
    if (semctl(semid, 0, SETALL, arg) == -1)
        ErrExit("semctl SETALL fallita\n");
    //apertura FIFO1 e FIFO2 in lettura 
     
    FIFO1id = open(pathnameFIFO1, O_RDONLY| O_NONBLOCK); //non bloccante
    if(FIFO1id == -1) {
        ErrExit("open fallito\n");
    }
     
    FIFO2id = open(pathnameFIFO2 , O_RDONLY | O_NONBLOCK); //non bloccante
    if(FIFO2id == -1)
        ErrExit("errore apertura FIFO2\n");

    while(1) {
    
        size_t mSize; //size mshq
        char r[100]; // variabile buffer del numero dei file che abbiamo letto
        semOp(semid , 1, -1); //aspetto che il client scriva sulla fifo 1 il n. di file
        // lettura messaggio da FIFO1 e scrittura messaggio su shared_memory

        int fifo = read(FIFO1id , r , sizeof(r));
   
        if(fifo == -1) {
            ErrExit("errore lettura numero file su FIFO1\n");
        }

        //converto il numero dato come stringa ad intero
        int n = atoi(r);

        printf("numero file %d\n",n);

        //messaggio da scrivere su shared memory
        char stringa[100] = "numero ricevuto"; 
    
        strcpy(ptr_shm ->parte_da_inviare ,stringa);
        semOp(semid, 0, 1); // sblocco client 
        semOp(semid, 1, -1); //aspetto che il client legga dalla shared memory
    
        //ricezione file, mi salvo i messaggi
        message_t messaggio ;

        //tengo conto di quando mi arrivano tutte e quattro le parti di un file
        int c[n+1];

        for(int i = 0;i < n+1; i++){
            c[i] = 0;
        }

        semOp(semid,1,-1); //aspetto che i figli scrivano sui vari canali
    
        int num_file = 0 ; 
    
        if(num_file < n ){
        
            //leggo da fifo1 e salvo dentro messaggio
            int fifo1 = read(FIFO1id , &messaggio, sizeof(messaggio));

            if(fifo1 == -1 ) 
                ErrExit("errore nella lettura della FIFO1\n");

            //posiziono il messaggio nel "grande array"
            posiziona_messaggio(&messaggio, n ,num_file,c) ;
            printf("messaggi letto su fifo1\n");
      
            semOp(semid,2,1);  //sblocco entrata in fifo1
      
            int fifo2 = read(FIFO2id, &messaggio, sizeof(messaggio));
            if(fifo2 == -1)
                ErrExit("errore nella lettura della FIFO2\n");
        
            posiziona_messaggio(&messaggio, n ,num_file,c) ;
            semOp(semid,3,1);

            printf("messaggi letto su fifo2\n");

            //shared memory
            messaggio.mtype = ptr_shm->mtype;
            strcpy(messaggio.nome_file ,ptr_shm->nome_file);
            strcpy(messaggio.parte_da_inviare, ptr_shm->parte_da_inviare);
            messaggio.parte=ptr_shm->parte;
            messaggio.pid_mittente=ptr_shm->pid_mittente;
      
            semOp(semid , 4, 1); 
        
            posiziona_messaggio(&messaggio, n ,num_file,c) ;
            printf("messaggi letto su shared\n");

            //msg queue
            mSize = sizeof(message_t) - sizeof(long);
            if(msgrcv(mesgid, &messaggio , mSize , 0,IPC_NOWAIT) == -1)
                ErrExit("errore lettura message queue\n");
           
            semOp(semid, 5, 1);
         
            posiziona_messaggio(&messaggio, n ,num_file,c) ;
            printf("messaggi letto su message\n");

            printf("%d\n",num_file);
      
        }
    
        struct terminato termina ; 
    
        // invio messaggio finale 
    
        termina.mtype = n;
        strcpy (termina.text, "terminato");
    
        size_t size = sizeof(termina)-sizeof(long);
        if(msgsnd(mesgid , &termina , size , 0) == -1){ //bloccante finche non il client non ha letto
            ErrExit("errore invio mesage_queue\n");
        }
    }
    return 0;
}

/// @file sender_manager.c
/// @brief Contiene l'implementazione del sender_manager.

#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"
#include <fcntl.h>

#define semkey 1 // da settare uguale al serben
#define shmkey 2 // shared memori var globale
#define msgKey 3

#define pathnameFIFO1 "./fifo10"
#define pathnameFIFO2 "./fifo20"

int FIFO1id, FIFO2id, shmid, mesgid, sem;

void quit(int sig){
   
    if (shmid != 0 )
        remove_shared_memory(shmid);
   
    if(mesgid != 0 && (msgctl(mesgid, IPC_RMID, NULL)==-1) )
        ErrExit("message_queue non è stata chiusa");
   
    if (FIFO1id != 0 && close(FIFO1id) == -1)
        ErrExit("close failed");

    if (FIFO2id != 0 && close(FIFO2id) == -1)
        ErrExit("close failed");

    // Remove the FIFO
    if (unlink(pathnameFIFO1) != 0)
        ErrExit("unlink failed");
        
    if (unlink(pathnameFIFO2) != 0 )
        ErrExit("unlink failed");
    
    if(sem != 0 && (semctl(sem , 0 , IPC_RMID, NULL )== -1))
       ErrExit("set di semafori non eliminato");
       
    // terminate the process
    exit(0);
}

int main(int argc, char * argv[]) {

 // all'arrivo di SIGINT devo togliere tutte le IPC aperte 
    if (signal(SIGINT, quit) == SIG_ERR)
       ErrExit("change signal handlers failed"); 

   // ControllaCartelle();

    //creazione shared_memory , message_queue , FIFO1 , FIFO2 
   
    int shmid = alloc_shared_memory(shmkey, 4096);
    
    int mesgid = msgget(msgKey , IPC_CREAT | S_IRUSR | S_IWUSR);//coda di messaggi

    if(mesgid == -1) 
        ErrExit("message queue non creata");
    
    if(mkfifo(pathnameFIFO1, O_CREAT | S_IRUSR | S_IWUSR) == -1)  //fifo 1
        ErrExit("FIFO10 non creata");
    
    if(mkfifo(pathnameFIFO2, S_IRUSR | S_IWUSR) == -1) //fifo 2
        ErrExit("FIFO20 non creata");
        
    //apertura FIFO1 e FIFO2 in lettura 
   
    FIFO1id = open(pathnameFIFO1 , O_RDONLY);

    if(FIFO1id == -1)
        ErrExit("errore apertura FIFO1");
   
    FIFO2id = open(pathnameFIFO1 , O_RDONLY);

    if(FIFO2id == -1)
        ErrExit("errore apertura FIFO2");
        
    int n; // variabile buffer del numero dei file che abbiamo letto
   
    // lettura messaggio da FIFO1 e scrittura messaggio su shared_memory 
   
    int fifo = read(FIFO1id, n, sizeof(n));
   
    if(fifo == -1 ) 
        ErrExit("errore nella lettura della FIFO1");
 
    //creazione  semaforo  per gestione shared_memory 
    int semid = semget(semkey, 2, S_IRUSR | S_IWUSR);   
    union semun arg;
    unsigned short values[] = {0, 0}; // inizializzazione del set di semafori 
    arg.array = values;

    if (semctl(semid, 0, SETALL, arg) == -1)
        ErrExit("semctl SETALL failed");
   
    char *ptr_shm= (char *)get_shared_memory(shmid , 0);  
    ptr_shm = "numero file ricevuto";
  
    semOp(semid,0,1); // sblocco client 
   
    
    // apro fifo anche in scrittura per far si' che rimangano in ascolto sul canale 
    
    int FIFO1= open(pathnameFIFO1 , O_WRONLY);

    if(FIFO1== -1)
        ErrExit("errore apertura FIFO1 in scrittura");
    
    int FIFO2 = open(pathnameFIFO1 , O_WRONLY);

    if(FIFO2 == -1)
        ErrExit("errore apertura FIFO2 in scrittura");
    
    //ricezione file 
    
    message_t *messaggio ; 
    message_t **messaggi;

    messaggi = (int *)malloc(n * sizeof(int));

    for(int i = 0; i < n; i++){
        messaggi[i] = (int *)malloc(4 * sizeof(int));  
    }

    int *c;
    c = (int *) malloc(n * sizeof(int));

    for(int i = 0;i < n; i++){
        c[i] = 0;
    }
    
    int num_file = 0 ; 
    int g = 0 ; 
    
    //creazione set di semafori per gestire i 50 messaggi su ogni struttura 
    int sem = semget(sem, 4, S_IRUSR | S_IWUSR);   
    union semun ar;
    unsigned short value[] = {50, 50, 50 , 50 }; // inizializzazione del set di semafori 
    ar.array = value;

    if (semctl(sem, 0, SETALL, arg) == -1)
        ErrExit("semctl SETALL failed");
   
    
    while(num_file != n ){
        
       
        int fifo1 = read(FIFO1id , messaggio, sizeof(messaggio));
        if(fifo == -1 ) 
           ErrExit("errore nella lettura della FIFO1");
           
        semOp(sem, 0 , 1 ); 
        
        inserimento_messaggio(messaggio, n , messaggi, num_file, c) ;
    
        
        int fifo2 = read(FIFO2id, messaggio, sizeof(messaggio));
        if(fifo2 == -1)
           ErrExit("errore nella lettura della FIFO2");
           
        semOp(sem , 1, 1 ); 
        
        inserimento_messaggio(messaggio, n , messaggi, num_file,c );
    
        semOp(semid , 1 , -1 ); // attendo scrittura dati 
        
        ptr_shm = messaggio; 
        
        semOp(sem , 2, 1); 
        
        inserimento_messaggio(messaggio, n , messaggi, num_file,c );
    
       
        size_t mSize = sizeof( message_t ) - sizeof(long);
        if(msgrcv(mesgid, &messaggio , mSize , 0 )==-1)
           ErrExit("errore lettura message queue");
           
        semOp(sem, 3, 1);
         
        inserimento_messaggio(messaggio, n , messaggi, num_file, c );

    }
      
    free_shared_memory(ptr_shm);
    
   // rimuovo semaforo per gestire shared_memory 
    if (semctl(semid, 0 /*ignored*/, IPC_RMID, NULL) == -1)
        ErrExit("semctl IPC_RMID failed"); 
   
   //struttura per invio messaggio sulla message queue 
    struct terminato termina ; 
    
    // invio messaggio finale 
    
    termina.mtype = 1 ;
    termina.text = " terminato " ; 
    size_t size = sizeof(struct terminato) - sizeof(long);
    if ( msgsnd(mesgid, &termina, size, 0 ) == -1)
         ErrExit("errore invio mesage_queue");
     
    if(close(FIFO1) == -1)
       ErrExit("chisura fifo1 in scrittura non riuscita");
    
    if(close(FIFO2) == -1)
       ErrExit("chiusura fifo2 in scrittura non riuscita");
    
   //deve ritornare su fifo1 in ascolto se non arrriva SIGINT 
}
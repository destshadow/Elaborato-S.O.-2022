/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "defines.h"

#define pathnameFIFO1 "./fifo10"
#define pathnameFIFO2 "./fifo20"

#define semkey 1 // da settare uguale al serben
#define shmkey 2 // shared memori var globale
#define msgKey 3

sigset_t prevSet;

void sigHandler(int sig){

    if(sig == SIGUSR1){
        exit(0);
    }
    
    if(sig == SIGINT ){
        if(sigprocmask( SIG_SETMASK, &prevSet, NULL) == -1) // serve per controllare se va in errore la maschera
            ErrExit("Errore nel settare la maschera");
    }
}

int main(int argc, char * argv[]) {

    char *nomi;
    char *caratteri;

    nomi = malloc(4 * sizeof(char));
    
    if(argc <= 1){
        printf("Errore non hai passato un path\n");
        return 0;
    }

    char *PathToSet = argv[1]; //chdir(PathToSet);

    // cambio maschera 
    sigset_t mySet;
    
    //inizializzo mySet con tutti i segnali
    if(sigfillset(&mySet) == -1){
        ErrExit("maschera non creata");
    }

    // rimuovo tutti i segnali tranne SIGINT e SIGUSR1
    if(sigdelset(&mySet, SIGINT) == -1 ){
        ErrExit("Errore nel settare singint");
    }

    if(sigdelset(&mySet, SIGUSR1) == -1){
        ErrExit("Errore nel settare sigusr1");
    }

    //blocco tutti i segnali alla maschera my_set
    if(sigprocmask( SIG_SETMASK, &mySet, &prevSet) == -1){
       ErrExit("Errore nel settare la maschera");
    }

    if (signal(SIGINT, sigHandler) == SIG_ERR ||
        signal(SIGUSR1, sigHandler) == SIG_ERR) {
        
        ErrExit("change signal handler failed");
    }

    ControllaCartelle();

    int count = ChangeDirAndGetEntry(PathToSet, nomi);
    
    int FIFO1id = open(pathnameFIFO1, O_WRONLY);

    if(FIFO1id == -1)
       ErrExit("errore apertura FIFO1");
    
    int FIFO2id = open(pathnameFIFO2, O_WRONLY);

    if(FIFO2id == -1)
       ErrExit("errore apertura FIFO1");
    
    int mesgid = msgget(msgKey, S_IRUSR | S_IWUSR);
    if (mesgid == -1)
        ErrExit("msgget failed");
        
    int shmid = alloc_shared_memory(shmkey, 4096);

    if (write(FIFO1id, count, sizeof(count)) != sizeof(count)){
        ErrExit("write failed");
    }
    
    int sem = semget(semkey, 2, S_IRUSR | S_IWUSR);
    semOp(sem , 1 , 1 ); // sblocco server 
    semOp(sem , 0 ,-1); // attendo i dati 
    
    char *ptr_shm = (char *)get_shared_memory(shmid , 0);
    
    free_shared_memory(ptr_shm);

    // set di semafori per aspettare che i figli arrivino tutti allo stesso punto 
    
    int semid = semget(IPC_PRIVATE, count, S_IRUSR | S_IWUSR);
    if (semid == -1)
        ErrExit("semget failed");

    // Initialize the semaphore set
    unsigned short semVal[] = {1}; // inizializzo il set di semafori parzialmente così i restanti vengono settati a 0 
    union semun arg;
    arg.array = &semVal;

    if (semctl(semid, 0 /*ignored*/, SETALL, arg) == -1){
        ErrExit("semctl SETALL failed");
    }


    for(int i = 0; i < count; i++){
        //fork per ogni dato;

        pid_t pid = fork();
        if(pid == -1){
            ErrExit("errore fork");
        }
        
        if(pid==0){
        
            message_t *messaggio;
            
            
            int fd = open( &nomi[i], O_RDONLY);

            if(fd == -1 ){
                ErrExit("errore nell'apertura del file");
            }
            ssize_t numChar = read(fd, caratteri, 4096);

            if(numChar == -1){
                ErrExit("errore lettura caratteri");
            }

            divisione_parti(numChar , parti, fd);
            semOp(semid, (unsigned short)i, -1);
           
            if(i+1 < count){
                semOp(semid, (unsigned short)  i+1 , 1); //sblocco prossimo semaforo
            }else{ 
                if (semctl(semid, 0 /*ignored*/, IPC_RMID, NULL) == -1){ //se sono in fondo (non ho piu figli) tolgo il set dei semfori a tutti per sbloccarli
                    ErrExit("semctl IPC_RMID failed");
                }
            }  
            int semafori = semget(sem, 4, S_IRUSR | S_IWUSR);
                    
            for(int k = 0 ; k<4 ; k++){
            
                messaggio ->mtype = 0 ;
                messaggio-> pid_mittente = getpid();
                messaggio-> nome_file = nomi[i]; 
                messaggio-> num_parte = k ;
                messaggio -> parte_da_inviare = &parti[k] ; 
                
                switch (k) {
                
                    case 0 : 
                            if (write(FIFO1id, messaggio, sizeof(messaggio)) != sizeof(messaggio)){
                                ErrExit("write failed");
                            } 
                            semOp(semafori, 0 , -1);  
                            break ;
                    
                    case 1 :
                            if (write(FIFO2id, messaggio, sizeof(messaggio)) != sizeof(messaggio)){
                                ErrExit("write failed");
                            }
                            semOp(semafori, 1 , -1);
                            
                        break; 
                            
                    case 2: 
                            ptr_shm = messaggio;
                            semOp(semid , 1 , 1 ) ;
                            semOp(semafori, 2 , -1);
                        break;
                    
                    case 3: 
                            messaggio -> mtype = 1 ; 
                            size_t mSize = sizeof(message_t) - sizeof(long);
                            if(msgsnd(mesgid , &messaggio , mSize , 0) == -1){
                                ErrExit("msgsnd failed");
                            }
                            semOp(semafori , 3, -1);
                        break ;
                    default:
                            ErrExit("qualcosa è andato storto nello switch");
                        break;
                }
            }
             close(fd);
             exit(0);
    }
  }
  
    while (wait(NULL) != -1); // padre attende la terminazione dei figli 

    struct terminato termina;

    size_t size = sizeof(termina) - sizeof(long);
    if ( msgrcv(mesgid, &termina, size, 0, 0 ) == -1){
        ErrExit("messaggio non ricevuto");
    }
   //deve ritornare a settare la maschera 
}
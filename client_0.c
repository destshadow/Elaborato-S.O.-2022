#include "defines.h"
#include "err_exit.h"
#include "shared_memory.h"
#include <sys/wait.h>

//definizione chiavi per semafori ,shared_memory e message_queue

#define  semkey 10 
#define  shmkey 20 // shared memori var globale
#define  msgKey 30 

//massimo numero di byte di un file 
#define MAX 4096

// Nomi fifo
char *pathnameFIFO1 = "fifo1";
char *pathnameFIFO2 = "fifo2";

//maschera di segnali
sigset_t mySet;

//puntatore alla struttura message_t per la shared_memory
message_t *ptr_shm;

void sigHandler(int sig){
  
    sigset_t prevSet;
  
    if(sig == SIGUSR1){
        exit(0);
    }
    
    if(sig == SIGINT ){

        if(sigfillset(&prevSet) == -1)
            ErrExit("maschera non creata\n");
      
        if(sigprocmask( SIG_SETMASK, &prevSet, NULL) == -1) // serve per controllare se va in errore la maschera
            ErrExit("Errore nel settare la maschera\n");
    }
}


int main(int argc, char * argv[]) {
  
    if(argc < 1) {
        printf("Errore non hai passato un path\n");
        return 0;
    }
    char *PathToSet = argv[1]; //PathToSet è il path passato dall'utente;
    char *path = getenv("PWD");
  
    while(1) {
      
        Crea_maschera(mySet); //crea la maschera di segnali 

        if(signal(SIGUSR1, sigHandler) == SIG_ERR || signal(SIGINT, sigHandler) == SIG_ERR) {
            ErrExit("Cambio del gestore fallito\n");
        }

        if(pause() == -1){
            // Blocca tutti i segnali compresi SIGINT e SIGUSR1
            if(sigprocmask(SIG_SETMASK, &mySet, NULL) == -1) {
                ErrExit("sigprocmask fallito\n");
            }
        }

        //spostamento path indicato
        if(chdir(PathToSet) == -1){
            ErrExit("Errore nel cambio directory\n");
        }

        char *pwd = getcwd(NULL,0);
        printf("\nCiao %s, ora inizio l’invio dei file contenuti in %s\n", getenv("USER"), pwd);
   
        char *nomi[100]; //memorizza il path dei file
        //conta il numero di file presenti
        int count = ChangeDirAndGetEntry(0,PathToSet, nomi);

        char buffer [100]; //memorizza numero di file presenti che soddisfano le condizioni
        sprintf(buffer,"%d",count);//converto in stringa
  
        if(chdir(path)==-1) //ritorno al path vecchio senno non trova piu dove sono le fifo e le altre cose
            ErrExit("cambio directory non riuscita\n");
   
        int shmid = alloc_shared_memory(shmkey, MAX); //creo la shared 
        ptr_shm= get_shared_memory(shmid, 0); //faccio share map

        printf("Apertura della FIFO1\n");
        int FIFO1id = open(pathnameFIFO1, O_WRONLY | O_NONBLOCK); //in scrittura non bloccante

        if(FIFO1id == -1){
            ErrExit("open fallito\n");
        }
   
        printf("scrivo su fifo1 il numero di file\n");
        int writeNum = write(FIFO1id, buffer, sizeof(buffer)); // scrivo sulla fifo il numero

        if(writeNum != sizeof(buffer)){
            ErrExit("write failed\n");
        }

        //creo set di semafori per gestione client-server e per gestione messaggi
        //0 client 1 server gli altri 4 sono per le fifo1 fifo2 shm e msq che gestiscono i 50 messaggi
        int sem = semget(semkey, 6, S_IRUSR | S_IWUSR);
        if(sem==-1){
            ErrExit("semaforo shared_memory non creato\n");
        }
  
        semOp(sem, 1 , 1); //sblocco server
        semOp(sem , 0 , -1) ;//Aspetto che il server scriva sulla shared memory 
         
        if(ptr_shm != NULL)
            printf("messaggio ricevuto\n");

        semOp(sem, 1, 1); //sblocco server dopo aver ricevuto messaggio su shared_memory

        //apro fifo2 ,message,queue,
        int FIFO2id = open(pathnameFIFO2, O_WRONLY|O_NONBLOCK);

        if(FIFO2id == -1)
            ErrExit("errore apertura FIFO2\n");
    
        int mesgid = msgget(msgKey, S_IRUSR | S_IWUSR);
        if (mesgid == -1)
            ErrExit("msgget failed\n");

        //creo set di semafori per gestire i figli   
        int semafori = crea_set_di_semafori(count);
      
        printf("inizio creazione figli\n");
   
        // Crea n processi figlio
        // -------------------------------------------------------
   
        for(int i = 0 ; i < count ; i++) {
            pid_t pid = fork();     // Crea processo figlio

            if(pid == -1) {
                ErrExit("errore fork\n");
            } else if(pid == 0) {
                // Esecuzione processo figlio

                // Strutture per ogni canale
                message_t messaggio1;
                message_t messaggio2;
                message_t messaggio3;
                message_t messaggio4;
              
                //buffer dei caratteri del singolo file
                char caratteri[MAX+1];
                ssize_t numChar;

                int fd = open(nomi[i], O_RDONLY, S_IRWXU); //apro il file
                if(fd == -1) {
                    ErrExit("apertura file fallita\n");
                }

                //conto numero di caratteri e li storo
                if((numChar = read(fd, caratteri, MAX)) == -1) {
                    ErrExit("lettura file fallita\n");
                }

                caratteri[numChar] = '\0'; //aggiungo carattere di terminazione

                // Si divide il file in 4 parti
                int start = 0; //da dove inizio
                int offset = numChar/4; //quanti caratteri devo leggere
                if(numChar % 4 != 0 )
                    offset++;
              
                for(int j= 0 ; j <4 ;j++){
                    if(numChar <= 9){
                        offset = (numChar/ 4);
                        if(j < (numChar% 4)){
                            offset++;
                        }
                    }
                    //divido il contenuto nei vari canali
                    switch(j){
                        case 0:
                            strncpy(messaggio1.parte_da_inviare, caratteri +start, offset);
                            messaggio1.parte_da_inviare[offset]='\0';
                            messaggio1.mtype = j;
                            messaggio1.pid_mittente = getpid();
                            strcpy(messaggio1.nome_file,nomi[i]);
                            messaggio1.parte = j;
                            break;

                        case 1:
                            strncpy(messaggio2.parte_da_inviare, caratteri +start, offset);
                            messaggio2.parte_da_inviare[offset]='\0';
                            messaggio2.mtype = j;
                            messaggio2.pid_mittente = getpid();
                            strcpy(messaggio2.nome_file,nomi[i]);
                            messaggio2.parte = j;
                            break;

                        case 2: 
                            strncpy(messaggio3.parte_da_inviare, caratteri +start, offset);
                            messaggio3.parte_da_inviare[offset]='\0';
                            messaggio3.mtype = j;
                            messaggio3.pid_mittente = getpid();
                            strcpy(messaggio3.nome_file,nomi[i]);
                            messaggio3.parte = j ;
                            break;

                        case 3:
                            strncpy(messaggio4.parte_da_inviare, caratteri +start, offset);
                            messaggio4.parte_da_inviare[offset]='\0';
                            messaggio4.mtype = j;
                            messaggio4.pid_mittente = getpid();
                            strcpy(messaggio4.nome_file,nomi[i]);
                            messaggio4.parte = j ;
                            break;
                        default:
                            break;
                    }

                    start+=offset; //aggiorno start ad ogni iterazione(da dove parto)
                }
                

                // figlio si blocca e sblocca il prossimo se c'è
                controlla_prossimo(i,count,semafori);
                
                for(int k = 0 ; k<4 ; k++){
                  
                    size_t mSize;
                
                    switch (k) {
                
                    case 0 : 
                            if (write(FIFO1id, &messaggio1, sizeof(messaggio1)) != sizeof(messaggio1)){
                                ErrExit("write failed");
                            } 
                            printf("messaggio scritto su fifo1\n");
                         
                            semOp(sem, 2 , -1);  
                            break ;
                    
                    case 1 :
                            if (write(FIFO2id, &messaggio2, sizeof(messaggio2)) != sizeof(messaggio2)){
                                ErrExit("write failed");
                            }
                            printf("messaggio scritto su fifo2\n");
                           
                            semOp(sem, 3, -1);
                            
                        break; 
                            
                    case 2:
                            //shared memory
                            ptr_shm->mtype=messaggio3.mtype;
                            strcpy(ptr_shm->nome_file ,messaggio3.nome_file);
                            strcpy(ptr_shm ->parte_da_inviare, messaggio3.parte_da_inviare);
                            ptr_shm->parte=messaggio3.parte;
                            ptr_shm->pid_mittente = messaggio3.pid_mittente;                      
                            printf("messaggio scritto su shared_memory\n");
                          
                            semOp(sem, 4 , -1);
                        break;
                    
                    case 3: 
                            mSize = sizeof(message_t) - sizeof(long);
                      
                            if(msgsnd(mesgid , &messaggio4 , mSize , IPC_NOWAIT) == -1) //non si deve bloccare
                                ErrExit("msgsnd failed");

                            printf("messaggio scritto su message queue\n");
                         
                            semOp(sem, 5, -1);
                        break ;
                    default:
                            ErrExit("qualcosa è andato storto nello switch");
                        break;
                }
            }
            close(fd); //chiudo il file descriptor
            semOp(sem,1,1); // sblocca server invio messaggi fatto (avviso il server che ho scritto)
            //printf("chiuso"); //chiudo tutto 
            exit(0);
    }
}             
        while(wait(NULL) != -1); //padre attende la terminazione dei figli

        struct terminato termina; //
    
        printf("Messaggio di conferma message queue: \n");
        int size = sizeof(termina) - sizeof(long);

        if(msgrcv(mesgid, &termina, size, count , 0) == -1){ //si blocca così non mettiamo semafori per aspettare il client
            ErrExit("ricezione messaggio message queue non riuscita\n");
        }

      //rimozione shared_memory
        if (shmid != 0 ){
            free_shared_memory(ptr_shm);
            remove_shared_memory(shmid);
        }
    }
    return 0;
}

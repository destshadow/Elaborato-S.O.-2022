/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "defines.h"




int main(int argc, char * argv[]) {

    message_t *parti;
    char *caratteri;

    if(argc <= 1){
        printf("Errore non hai passato un path\n");
        return 0;
    }

    char *PathToSet = argv[1];

    //chdir(PathToSet);

    CambiaMaschera();

    int count = ChangeDirAndGetName(PathToSet, parti->nome_file);
    
    //pathnameFIFO1 <- da settare all'interno dell'elaborato
    
    int FIFO1id = open(pathnameFIFO1 , O_WRONLY);
    if(FIFO1id == -1)
       errExit("errore apertura FIFO1");


    if (write(FIFO1id, count, sizeof(count)) != sizeof(count)){
        errExit("write failed");
    }
    
    int semid = semget(semkey, 1, S_IRUSR | S_IWUSR);
    
    int shmid = alloc_shared_memory(shmkey, 4096);
    char *ptr_shm = (char *)get_shared_memory(shmid , 0);
    
    free_shared_memory(ptr_shm);

    int semid = semget(IPC_PRIVATE, count, S_IRUSR | S_IWUSR);
    if (semid == -1)
        errExit("semget failed");

    // Initialize the semaphore set
    unsigned short semVal[0] = 1;
    for(int i = 1 ; i< count ; i++)
        semVal[i] = 0 ; 
    
    union semun arg;
    arg.array = &semVal;

    if (semctl(semid, 0 /*ignored*/, SETALL, arg) == -1)
        errExit("semctl SETALL failed");

    //TO DO:

    for(int i = 0; i < count; i++){
        //fork per ogni dato;

        pid_t pid = fork();
        if(pid == -1)
            errExit("errore fork");
        
        if(pid==0){
            parti->pid_mittente = getpid();
            int fd = open(parti->nome_file , O_RDONLY);

            if(fd == -1 )
                errEXit("errore nell'apertura del file");

            ssize_t numChar = read(fd, caratteri, 4096);

            if(numChar == -1)
                errExit("errore lettura caratteri");

            divisione_parti(numChar , parti, fd);
            semOp(semid, (unsigned short)i, -1);
           
            if(i+1 < count)
                semOp(semid, (unsigned short)  i+1 , 1); //sblocco prossimo semaforo
            else 
                if (semctl(semid, 0 /*ignored*/, IPC_RMID, NULL) == -1) //se sono in fondo (non ho piu figli) tolgo il set dei semfori a tutti per sbloccarli
                    errExit("semctl IPC_RMID failed");
    }
  
    return 0;
}
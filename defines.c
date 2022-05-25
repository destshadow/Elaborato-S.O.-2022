/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"

int ChangeDirAndGetEntry(char *s, char *dati){
    DIR *directory;
    struct dirent *dentry;
    char *nomi;
    int count = 0;

    char **it = environ;

    HOME = *it;
    PWD = *(it + 1);
    USER = *(it + 2);


    if(chdir(s) == -1){
        errExit("Errore nel cambio directory\n");
        return -1;
    }

    directory = opendir(s);

    if(directory == NULL){
        return -1;
    }

    printf("Ciao %s, ora inizio l’invio dei file contenuti in %s", USER, PWD);

    /*while ( (dentry = readdir(dp)) != NULL ) {
        if (dentry->d_type == DT_REG)
            printf("Regular file: %s\n", dentry->d_name);
            //prendiamo tutti i file in una directori
            //noi vogliamo solo quelli che iniziano per sendme_
    }*/

    struct stat *statbuf;

    while ((dentry = readdir(directory) != NULL)){
         
        if (strcmp(dentry->d_name, ".") == 0 ||
            strcmp(dentry->d_name, "..") == 0){ 
            //prendiamo tutti i file in una directori
            //noi vogliamo solo quelli che iniziano per sendme_ 
            if (dentry->d_type == DT_REG)
                printf("Regular file: %s\n", dentry->d_name);
            
            if(stat(dentry -> d_name , &statbuf) == -1){
                errExit("errore nello stat");
                return -1;
            }
            if (statbuf->st_size < 4096){
                if(memcmp (dentry ->d_name , "sendme_", 7 ) == 0 ){
                    nomi[count] = dentry ->d_name ;
                    count ++ ; 
                }
            } 
        }
    }

    if(count <= 0){
        errEXit("File non trovati");
        return -1;
    }
    dati = nomi;

    close(directory);
    return count; //sarebbe il numero di file da inviare

    
}

void divisione_parti(int d, int fd, char *stringa ){
   
    for(int i = 0 ; i< 4 ; i++){

        message_t *a;
        a = (message_t *)malloc(sizeof(message_t));

        off_t offset = lseek(fd, (int) i*(d/4) , SEEK_SET);
        ssize_t caratteri_letti = read(fd, stringa, (int)d/4 );

        if(caratteri_letti == -1){ 
            errExit("errore lettura %d caratteri" , d);
        }
        b = a;
    }
}

void sigHandler(int sig){
    
    //da controllare con il sigint quando compileremo un giorno lontano
    //mettere sta roba in una funzione e chiamarla all'interno del sigint
    /*sigset_t  mySet;
   
    if(sig == SIGINT ) {
      
        if(sigfillset(&mySet) == -1 ) 
            errExit("errore aggiunta ");
       
         
        if(sigprocmask(SIG_SETMASK , &mySet , NULL) == -1)
            errExit("errore associazione maschera");
    }*/
    if(sig == SIGINT){
        print("Hello\n");
        //go da fare roba
        //blocca tutti i segnali (compresi SIGUSR1 e SIGINT) modificando la maschera
    }

    if(sig == SIGUSR1){
        exit(0);
    }
}

void CambiaMaschera(){

    sigset_t mySet;
    sigset_t prevSet;

    //inizializzo mySet con tutti i segnali
    sigfillset(&mySet);

    // rimuovo tutti i segnali tranne SIGINT e SIGUSR1
    if(sigdelset(&mySet, SIGINT) == -1 ){
        errExit("Errore nel settare singint\n");
    }

    if(sigdelset(&mySet, SIGUSR1) == -1){
        errExit("Errore nel settare sigusr1\n");
    }

    //blocco tutti i segnali alla maschera my_set
    sigprocmask( SIG_SETMASK, &mySet, &prevSet);

    if (signal(SIGINT, sigHandler) == SIG_ERR ||
        signal(SIGUSR1, sigHandler) == SIG_ERR) {
        
        errExit("change signal handler failed\n");
    }
}

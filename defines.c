/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"
#include "err_exit.h"

#define  semkey // da settare uguale al serben
#define  shmkey // shared memori var globale
#define  msgKey 

#define pathnameFIFO1 "./fifo10"
#define pathnameFIFO2 "./fifo20"

int ChangeDirAndGetEntry(int n ,char *s, char *nomi){
    DIR *directory;
    struct dirent *dentry;
    int count = 0;
    char *path ;
    int dimensione;
    struct stat statbuf;
    
    if(chdir(s) == -1){
        ErrExit("Errore nel cambio directory\n");
        return -1;
    }
   
    char *pwd = getcwd(NULL, 0); //restituisce il nome della directory
    printf("Ciao %s, ora inizio l’invio dei file contenuti in %s", getenv("USER"), pwd);
   
    directory = opendir(s);
    if(directory == NULL){
        return -1;
    }

    while ((dentry = readdir(directory)) != NULL){
        
        //controllo sottocartelle 
        
        if (dentry -> d_type == DT_DIR && strcmp(dentry->d_name, ".") != 0 ||
            strcmp(dentry->d_name, "..") != 0){ 
            
            dimensione = strlen(s) + strlen(dentry->d_name) + 2; //lunghezza path + nome entry
            
            path = (char *) malloc(sizeof(char) * dimensione);

            if(path == NULL) {
                ErrExit("malloc fallito");
            }

            // Imposta il nuovo path
            strcpy(path,s);
            strcat(strcat(path, "/"), dentry->d_name);
            
            // Chiamata ricorsiva
            int n = ChangeDirAndGetEntry(n, path, nomi);

            free(path);
        }
            
        if (dentry->d_type == DT_REG && strchr(dentry ->d_name , "sendme_") != NULL){
            printf("Regular file: %s\n", dentry->d_name);
            
            dimensione = strlen(s) + strlen(dentry->d_name) + 2;
            char *nome = (char *) malloc(sizeof(char) * dimensione);

            //  path del file 
            strcpy(nome, s);
            strcat(nome, "/");
            strcat(nome, dentry->d_name);

            //costruisco il path (riga sopra)
            
            if(stat(nome, &statbuf) == -1) {
                ErrExit("stat fallito");
            }
            
            if(statbuf.st_size >= 4 && statbuf.st_size <= 4096) {
               
               nomi[count] = (char *) malloc(sizeof(char) * dimensione);
               nomi[count] = nome;
               count ++ ; 
            }  
        }
    }
    
    if(count <= 0 || count > 100 ){
        ErrExit("File non trovati o ci sono più di 100 file ");
    }
 
    if( closedir(directory) == -1);
        ErrExit("closedir fallito");
   
    return count; // numero di file da inviare
}

//((myint+3)/4)*4

void divisione_parti(int d, char *parti , int fd ){

    d = ((d+3)/4)*4;
   
    for(int i = 0 ; i< 4 ; i++){

        off_t offset = lseek(fd, (int) i*(d/4) , SEEK_SET);
        ssize_t caratteri_letti = read(fd, &parti[i] , (int)d/4 );

        if(caratteri_letti == -1){ 
            ErrExit("errore lettura caratteri");
        }
    }
}

void inserimento_messaggio(message_t *messaggio, int n, message_t **messaggi, int num_file, int c[4]){

    int g = posiziona_messaggio(messaggio, n, messaggi, c); //positivo se ha tutte e 4 le parti del file

    if(g>0){
        stampa_su_file(g, n, messaggi);
        num_file++;
    }
}

//creo array di messaggi dentro al server e tramite i vari canali mi arriva un messaggio alla volta e lo metto dentro al

int posiziona_messaggio(message_t *messaggio, int count,  message_t **messaggi, int c[]){

    int r = 0 ; //righe
    int n = 0 ; //sempre 0
        
    for(r = 0 ;  r < count ; r++){
        
        if(messaggi[r][n].nome_file == NULL ){
            messaggi[r][n].nome_file  =  messaggio->nome_file; //quella riga è per quel file
            messaggi[r][messaggio ->num_parte].mtype =  messaggio->mtype;     //setto la parte a seconda di dove si strova
            messaggi[r][messaggio ->num_parte].pid_mittente =  messaggio->pid_mittente;
            messaggi[r][messaggio -> num_parte].nome_file  =  messaggio->nome_file;
            messaggi[r][messaggio -> num_parte].num_parte =  messaggio->num_parte;
            messaggi[r][messaggio -> num_parte].parte_da_inviare = messaggio->parte_da_inviare ;
            c[r]++;
            break;
        }else if(messaggi[r][n].nome_file == messaggio->nome_file){
                messaggi[r][messaggio->num_parte].mtype =  messaggio->mtype;
                messaggi[r][messaggio->num_parte].pid_mittente =  messaggio->pid_mittente;
                messaggi[r][messaggio->num_parte].nome_file  =  messaggio->nome_file;
                messaggi[r][messaggio->num_parte].num_parte =  messaggio->num_parte;
                messaggi[r][messaggio->num_parte].parte_da_inviare = messaggio->parte_da_inviare ;
                c[r]++;

                if(c[r]==4){
                    return r; //riga a cui si riferisce
                }
            break;
        }else{
            return -1;
        }
    }
}
      
void stampa_su_file ( int r , int count , message_t **messaggi){
      
    char *nome = strcat((char *) messaggi[r][0].nome_file  , "_out"); 
      
    int fd = open(nome , O_RDWR| O_CREAT |O_TRUNC|S_IRUSR|S_IWUSR);  
      
    for(int k = 0 ; k <4 ; k++){
          
        if(messaggi[r][k].num_parte == k ){
          
            char *buffer = scrivi(r, k, messaggi); 
            ssize_t numWrite = write(fd, buffer,sizeof(buffer));
            if(numWrite != sizeof(buffer))
                ErrExit("write failed");
        }
        if(close(fd) == -1){
            ErrExit("chiusura file non riuscita");
        }
    }
}
      
char * scrivi(int r , int k , message_t **messaggi){
      
    char *a_capo = "\n";
      
    char *buffer = "";
    sprintf(buffer, "%d", (messaggi[r][k].num_parte +1)  ); 
      
    char *parte = strcat("[Parte " , buffer);
    char *nome = strcat("del file" , messaggi[r][k].nome_file);
    sprintf(buffer, "%d" , messaggi[r][k].pid_mittente);
    char *pid = strcat("spedita dal processo " , buffer);
      
    switch(k) {
      
        case 0 : 
            buffer = "fifo1";
        break;
                
        case 1 : 
            buffer = "fifo2"; 
        break;
                
        case 2 :
            buffer = "shared_memory";
        break;
                
        default :
            buffer = "message_queue";
        break;
    }
       
    char *canale = strcat("tramite" , buffer);
     
    buffer = strcat(parte , nome );
    char *seconda = strcat(pid , canale);
     
    char *linea = strcat(buffer , seconda) ;
    char *prima_linea = strcat(linea , a_capo);
     
    char *testo = messaggi[r][k].parte_da_inviare;
     
    char *file = strcat(prima_linea , testo );
     
    char *res = strcat(file , a_capo);
     
    return res; 
}

void ControllaCartelle(){
    if(open(pathnameFIFO1, O_EXCL | O_CREAT | O_TRUNC | O_RDWR) == -1){
        ErrExit("cartella fifo1 non creata");
    }
    
    if(open(pathnameFIFO2, O_EXCL | O_CREAT | O_TRUNC | O_RDWR) == -1){
        ErrExit("cartella fifo1 non creata");
    }
}

/*char *GetSpecificPathEntry(int a){

    char *HOME;
    char *PWD;
    char *USER;
    char *s;

    extern char **environ;
    char **it = environ;

    //HOME = *it;
    //PWD = *(it + 1);
    //USER = *(it + 2);

    for (char **it = environ; (*it) != NULL; ++it) {
        s = *it;
        //printf("--> %s\n", s);
        
        if(strncmp(s,"PWD", 3) == 0){
            PWD = *it;
            strcpy(PWD, (s + 4));
        }

        if(strncmp(s, "HOME", 4) == 0){
            HOME = *it;
            strcpy(HOME, (s + 5));
        }

        //+ 1 per l'uguale sopra lo conto gia 
        if(strncmp(s, "USER", strlen("USER")) == 0){
            USER = *it;
            strcpy(USER, (s + strlen("USER") + 1));
        }
        
        //+ 1 per l'uguale sopra lo conto gia 
        if(strncmp(s, "USERNAME", strlen("USERNAME")) == 0){
            USER = *it;
            strcpy(USER, (s + strlen("USERNAME") + 1));
        }
    }

    if(a == 1){
        return HOME;
    }

    if(a == 2){
        return PWD;
    }
    
    if(a == 3){
        return USER;
    }

    return "Errore";
}*/
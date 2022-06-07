/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"
#include "err_exit.h"

#define  semkey 10// da settare uguale al serben
#define  shmkey 20// shared memori var globale
#define  msgKey 30

#define pathnameFIFO1 "obj/fifo10"
#define pathnameFIFO2 "obj/fifo20"


void Crea_maschera(sigset_t mySet){
  
    // Riempie mySet con tutti i segnali
    if(sigfillset(&mySet) == -1 ) {
        ErrExit("sigfillset fallito");
    }

    // Rimuove dall'insieme il segnale SIGINT e SIGUSR1
    if(sigdelset(&mySet, SIGINT) == -1 || sigdelset(&mySet, SIGUSR1) == -1   ) {
        ErrExit("sigdelset fallito");
    }

    // Blocca tutti i segnali eccetto SIGINT e SIGUSR1
    if(sigprocmask(SIG_SETMASK, &mySet, NULL) == -1) {
        ErrExit("sigprocmask fallito");
    }
}

int ChangeDirAndGetEntry(int num, char *old_path, char **nomi) {
    size_t dim;          // Dimensione percorso
    char *path;      // Nuovo percorso
    struct dirent *dentry;  // Struttura della cartella letta
    struct stat statbuf;
    
    DIR *dirp = opendir(old_path);
    if(dirp == NULL){
        ErrExit("opendir fallito");
    }

    // Legge la cartella
    while((dentry = readdir(dirp)) != NULL) {
      
        // Entra nelle sotto cartelle
        if(dentry->d_type == DT_DIR && strcmp(dentry->d_name, ".") != 0 && strcmp(dentry->d_name, "..") != 0) {
            
            // path nuovo
            dim = strlen(old_path) + strlen(dentry->d_name) + 2;
            
            path = (char *) malloc(sizeof(char) * dim);

            if(path == NULL) {
                ErrExit("malloc fallito");
            }
          
            //  nuovo path
            strcpy(path,old_path);
            strcat(strcat(path, "/"), dentry->d_name);
            
            // Chiamata ricorsiva
            num = ChangeDirAndGetEntry(num,path, nomi);

            free(path);
        }
    
        
        // È un file valido
        if(dentry->d_type == DT_REG && strncmp(dentry->d_name, "sendme_", 7) == 0) {
            
            // spazio del percorso del file
            dim = strlen(old_path) + strlen(dentry->d_name) + 2;
            char *temp = (char *) malloc(sizeof(char) * dim);

            // crea percorso
            strcpy(temp, old_path);
            strcat(temp, "/");
            strcat(temp, dentry->d_name);
            
            if(stat(temp, &statbuf) == -1) {
                ErrExit("stat fallito");
            }
            
            if(statbuf.st_size >= 4 && statbuf.st_size <= 4096) {
            	//alloca spazio
              	nomi[num] = (char *) malloc(sizeof(char) * dim);
                if(nomi[num] == NULL) {
                    ErrExit("malloc fallito");
                }
                 // Imposta il percorso
                strcpy(nomi[num], temp);

                // Incrementa i file validi
                num++;
            }
            free(temp);
        }
    }

    // Si chiude il path indicato
    if(closedir(dirp) == -1) {
        ErrExit("closedir fallito");
    }
  
  	if(num <= 0 || num >100){
		printf("ci sono %d messaggi\n" ,num);
		ErrExit("non ci sono file o ce ne sono più di 100");
    }
    return num;
}

//count = numero di file
int crea_set_di_semafori(int count){
  	int figli = 0 ;
  	if(count >0){
      	int figli = semget(IPC_PRIVATE,count,IPC_CREAT|S_IRUSR | S_IWUSR);  
		if(figli == -1)
			ErrExit("errore creazione semafori");
	
		union semun ar;
		unsigned short value[] = {1}; // inizializzazione del set di semafori 
		ar.array = value;

		if (semctl(figli, 0, SETALL, ar) == -1)
			ErrExit("semctl SETALL fallita");
    }
  	return figli;
} 

void controlla_prossimo(int i, int count , int figli){
    if(figli != 0){
		//se esiste un prossimo figlio
        if(i+1 < count){
            semOp(figli,i+1,1); //lo sblocco
        }else{
			//se non esiste sblocco tutti i figli in un unico momento
            if (semctl(figli, 0 /*ignored*/, IPC_RMID, NULL) == -1) //se sono in fondo (non ho piu figli) tolgo il set dei semfori a tutti per sbloccarli
               ErrExit("semctl IPC_RMID failed");
        }
    }
}

int ritorna_indice(message_t *messaggio,message_t fifo1[],message_t fifo2[],message_t shared[],message_t message[],int index,int count){
	//se la fifo 1 all'index tot ha lo stesso nome del msg che arriva vuol dire che è una parte del messaggio allora ritorna l'indice
    for(int j = 0 ; j< count+1 ; j++){
       	if(messaggio->nome_file == fifo1[j].nome_file ||messaggio->nome_file == fifo2[j].nome_file ||messaggio->nome_file == shared[j].nome_file || messaggio->nome_file == message[j].nome_file)
         	return j;
    }
	//senno è l'index + 1 
  	return index +1;
}

//arriva il pezzetto del file dal canale, count = il numero di file totale, num_file = num file finiti, c = array delle 4 parti
void posiziona_messaggio(message_t *messaggio, int count,int num_file, int c[]){

	//struttura del client per ogni canale
    message_t fifo1[count+4];
    message_t fifo2[count+4];
    message_t shared[count+4];
    message_t queue[count+4];
    int index = 0; //indice
    int i; //indice

    i = ritorna_indice(messaggio,fifo1,fifo2,shared,queue,index,count);

  	switch(messaggio->parte){
       
     	case 0 :
		  		strcpy(fifo1[i].nome_file, messaggio->nome_file);
              	strcpy(fifo1[i].parte_da_inviare, messaggio->parte_da_inviare);
              	fifo1[i].parte = messaggio->parte;
              	fifo1[i].pid_mittente =messaggio->pid_mittente;
              	c[i]++;
            break;
    	case 1:
				strcpy(fifo2[i].nome_file, messaggio->nome_file);
              	strcpy(fifo2[i].parte_da_inviare, messaggio->parte_da_inviare);
              	fifo2[i].parte = messaggio->parte;
              	fifo2[i].pid_mittente =messaggio->pid_mittente;
              	c[i]++;
            break;
       	case 2:
	   			strcpy(shared[i].nome_file, messaggio->nome_file);
              	strcpy(shared[i].parte_da_inviare, messaggio->parte_da_inviare);
              	shared[i].parte = messaggio->parte;
              	shared[i].pid_mittente =messaggio->pid_mittente;
              	c[i]++;
            break;
     	case 3:
	 			strcpy(queue[i].nome_file, messaggio->nome_file);
              	strcpy(queue[i].parte_da_inviare, messaggio->parte_da_inviare);
            	queue[i].parte = messaggio->parte;
              	queue[i].pid_mittente =messaggio->pid_mittente;
              	c[i]++; //incremento una parte dell'array
            break;
       	default:
            	printf("qualcosa non è andato a buon fine\n");
            break;
   	}

   	if(c[i] == 4){ //ho completato il file
        printf("%d\n",c[i]);
        stampa_su_file(fifo1,fifo2,shared,queue,i);
        num_file++;
        printf("%d\n",num_file);
    }
}

void Extension(char *path) {
    char *ret = strrchr(path, '.');

    if(ret != NULL) {
        char *ext = (char *) malloc(sizeof(char) * strlen(ret) + 1);
        if(ext == NULL) {
            ErrExit("malloc fallito");
        }

        strcpy(ext, ret);
        strcpy(ret, "_out");
        strcat(path, ext);
        free(ext);
    } else {
        strcat(path, "_out");
    }
}

//ho bisogno dei 4 array
void stampa_su_file (message_t fifo1[], message_t fifo2[],message_t shared[],message_t message[],int index){
      
    char path[150] = ""; //memorizzo il path del file

    strcpy(path ,fifo1[index].nome_file);
    Extension(path); //gli aggiungo _out

    int fd = open(path, O_WRONLY | O_APPEND | O_TRUNC | O_CREAT , S_IRWXU); //apro il file
  
    if(fd == -1) {
        ErrExit("open fallito");
    }

	//il buffer dove memorizzo le parti da scrivere
  	char buffer[1500] = "";
    ssize_t numWrite;
  
	//scrivo ogni parte
    for(int k = 0 ; k <4 ; k++){
          
       	switch (k){
         
        	case 0: 
					scrivi(buffer, k, fifo1,index, "fifo1"); 
					numWrite = write(fd, buffer,strlen(buffer));
					if(numWrite != strlen(buffer))
						ErrExit("write failed");
         		break;
         	case 1:
					scrivi(buffer,k, fifo2,index,"fifo2"); 
					numWrite = write(fd, buffer,strlen(buffer));
					if(numWrite != strlen(buffer))
						ErrExit("write failed");
         		break;
         	case 2: 
					scrivi(buffer,k, shared,index,"shared_memory"); 
					numWrite = write(fd, buffer,strlen(buffer));
					if(numWrite != strlen(buffer))
						ErrExit("write failed");
         		break;
         	case 3: 
					scrivi(buffer, k, message,index,"message_queue"); 
					numWrite = write(fd, buffer,strlen(buffer));
					if(numWrite != strlen(buffer))
						ErrExit("write failed");
         		break;
        }
    }
    if(close(fd) == -1){
        ErrExit("chiusura file non riuscita");
	}
}

void scrivi(char buffer[],int k,message_t messaggio[] ,int index, char stringa[]){

	//Scrivo
	sprintf(buffer, "[Parte %d, del file %s, spedita dal processo %d tramite %s]\n%s\n\n", messaggio[index].parte +1,messaggio[index].nome_file , messaggio[index].pid_mittente, stringa,messaggio[index].parte_da_inviare);

}
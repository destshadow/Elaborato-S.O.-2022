/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once
#include <string.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "fifo.h"
#include <sys/ipc.h>
#include <sys/sem.h>
#include "semaphore.h"
#include <sys/stat.h>


struct message{
    long mtype ; 
    pid_t pid_mittente;
    char nome_file[100];
    int parte ;
    char parte_da_inviare[1025];
};

struct terminato{
    long mtype ; 
    char text[150];
};
        

typedef struct message message_t;


void Crea_maschera(sigset_t );
int ChangeDirAndGetEntry(int, char *, char **);
int crea_set_di_semafori(int count);
void controlla_prossimo(int i, int count , int figli);
void posiziona_messaggio(message_t *messaggio, int count,int num_file,int c[]);
void stampa_su_file ( message_t fiof1[],message_t fifo2[],message_t shared[],message_t message[],int index);
void scrivi(char buffer[], int k,message_t messaggio[] ,int index,char stringa[]);
void Extension(char *path);
/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

#include <string.h>
#include <sys/types.h>
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

char *HOME;
char *PWD;
char *USER;

extern char **environ;

struct message{
    long mtype ; 
    pid_t pid_mittente;
    char *nome_file;
    int num_parte;
    char *parte_da_inviare;
};

struct terminato{
    long mtype ; 
    char *text;
};
        
char *parti ;

typedef struct message message_t;


int ChangeDirAndGetEntry(char *, char *);
void divisione_parti(int, char *, int);
void inserimento_messaggio(message_t *messaggio, int n, message_t **messaggi, int num_file, int c[4]);
int  posiziona_messaggio(message_t *messaggio, int count,  message_t **messaggi, int c[]);
void stampa_su_file ( int r , int count , message_t **messaggi);
char * scrivi(int r , int k , message_t **messaggi);
void ControllaCartelle();
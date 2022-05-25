/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fifo.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "semaphore.h"
#include <sys/stat.h>

key_t semkey; // da settare uguale al serben
key_t shmkey; // shared memori var globale
key_t msgkey;

char *pathnameFIFO1 = "./fifo1";
char *pathnameFIFO2 = "./fifo2";

char *HOME;
char *PWD;
char *USER;

extern char **environ;

struct message{
    pid_t pid_mittente;
    char *nome_file;
    char *parte_da_inviare;
};

char *parti = {"", "", "", ""};

typedef struct message message_t;

int ChangeDirAndGetEntry(char *, char *);
void divisione_parti(int, message_t *, int);
void sigHandler(int );
void CambiaMaschera();
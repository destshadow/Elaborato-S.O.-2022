/// @file shared_memory.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione della MEMORIA CONDIVISA.

#pragma once
#include <sys/shm.h>
#include <signal.h>
#include <sys/ipc.h>

int alloc_shared_memory(key_t , size_t);
void *get_shared_memory(int , int);
void remove_shared_memory(int );
void free_shared_memory(void *);
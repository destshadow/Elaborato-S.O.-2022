/// @file fifo.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione delle FIFO.

#include "err_exit.h"
#include "fifo.h"

void rimozione_fifo(int id){
   if(close(id)==-1)
      ErrExit("chiusura fifo non è andata a buon fine");
}

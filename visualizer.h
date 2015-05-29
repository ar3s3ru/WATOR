/** 
    \file visualizer.h
    \author Danilo Cianfrone, matricola 501292

    Il programma qui presente è, in ogni sua parte, opera originale dell'autore.
    Danilo Cianfrone.
 */
    
#include "utils.h"

#ifdef __UTILS_H__
#define __VISUALIZER__

extern int errno;

/** Mette in atto la terminazione gentile,
    terminando il loop principale il prima possibile.
 */
static void sigint_handler();

/** Mette in atto la terminazione gentile,
    terminando il loop principale il prima possibile.
 */
static void sigterm_handler();

/** Permette il dumping del ciclo immediatamente
    successivo al catch del segnale sul file 
    specificato da argv[1].
 */
static void sigusr1_handler();

/** Indica al processo visualizer la possibilità di
    proseguire nel loop principale, stampando il
    pianeta su stdout o su dumpfile (SIGUSR1).
 */
static void sigusr2_handler();

/** Funzione di cleanup all'invocazione della exit().
    Il programma tiene traccia delle risorse allocate
    attraverso un numero definito di "step".

    La exit_cleaner() esegue il cleanup in relazione
    alle risorse generate durante l'esecuzione del 
    processo.

    Inoltre, manda SIGUSR2 al processo wator per
    comunicare l'uscita avvenuta.
 */
static void exit_cleaner();

/** Stampa su file stream il contenuto del buffer.
    
    \param stream: stream del file su cui stampare.
    \param buffer: buffer del socket.
    \param nrow: numero di righe della matrice.
    \param ncol: numero di colonne della matrice.

    \retval 0: stampa con successo.
    \retval -1: parametri errati (EINVAL)
 */
inline static int update_screen(FILE *stream, char *buffer, 
    unsigned int nrow, unsigned int ncol);

#endif /* __UTILS_H__ */
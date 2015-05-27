#include "utils.h"

#ifdef __UTILS_H__
#define __VISUALIZER__

extern int errno;

static void sigint_handler();
static void sigterm_handler();
static void sigusr1_handler();
static void sigusr2_handler();

/** Funzione di cleanup all'invocazione della exit() */
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
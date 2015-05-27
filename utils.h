#include <stdio.h>
#include <stdlib.h>
#include <errno.h>      /* Per l'errno */
#include <assert.h>     /* Debugging   */
#include <unistd.h>
#include <fcntl.h>      /* Files, open() */
#include <dirent.h>     /* Directories */
#include <signal.h>     /* Segnali */
#include <string.h>     /* Stringhe, memset() */
#include <pthread.h>    /* POSIX Thread */
#include <sys/socket.h> /* Socket */
#include <sys/types.h>
#include <sys/stat.h>   /* mkdir() */
#include <sys/un.h>     /* Sockaddr_un */

#include "wator.h"

#ifdef __WATOR__H
#define __UTILS_H__

/* Nome della socket */
#define SOCKNAME "./tmp/visual.sck"
/* Workers di default */
#define NWORK_DEF 2
/* Chronon di default */
#define CHRON_DEF 5

#define UNIX_PATH_MAX 108

/** Viene controllato il valore di ritorno della funzione.
    Se uguale a "checkval", esce con errno.

    \param fun: funzione da testare.
    \param checkval: valore da testare.
    \param msg: messaggio da stampare su stderr.
 */
#define err_check(fun, checkval, msg) \
   if (fun == checkval) { perror(msg); exit(errno); }

/** Controlla che la condizione cond valga -1;
    in caso affermativo, ritorna il valore retval.

    \param cond: condizione da testare.
    \param retval: valore di ritorno.

    \retval retval.
 */
#define err_check_min1(cond, retval)  \
   if (cond == -1) return retval;

/** Controlla che la condizione cond valga NULL;
    in caso affermativo, ritorna il valore retval.

    \param cond: condizione da testare.
    \param retval: valore di ritorno.

    \retval retval.
 */
#define err_check_null(cond, retval)  \
   if (cond == NULL) return retval;

/** Struttura per i rettangoli da usare nel worker. */
typedef struct _rect 
{
    point_t p1; /* Punto estremo sinistro */
    point_t p2; /* Punto estrema destra */
} rect_t;

/** Struttura che definisce l'oggetto di una coda */
typedef struct _queue_el
{
    rect_t info;            /* Info dell'elemento */
    struct _queue_el *next; /* Puntatore el. successivo */
} qelem_t;

/** Struttura per l'implementazione di una
    coda, politica FIFO.
 */
typedef struct _queue 
{
    qelem_t *head;   /* Puntatore alla testa */
    qelem_t *tail;   /* Puntatore alla coda */
    int     elem;   /* Numero di elementi in coda */
} queue_t;

/** Indica se la struttura puntata da *queue
    sia vuota o meno.

    \param queue: puntatore alla coda.

    \retval 0: se non è vuota
    \retval -1: parametro errato (errno = EINVAL).
    \retval =! 0 altrimenti.
 */
inline int isempty_queue(queue_t *queue);

/** Inserisce l'elemento puntato da elem in
    coda alla struttura puntata da queue.

    \param elem: puntatore all'elemento da aggiungere in coda.
    \param queue: puntatore alla coda.

    \retval -1: errore, parametri errati (EINVAL).
    \retval 0: elemento correttamente inserito.
 */
int push_queue(rect_t elem, queue_t *queue);

/** Estrae l'elemento in testa alla coda puntata
    da queue.

    \param queue: puntatore alla coda.

    \retval NULL: parametro errato o coda vuota.
    \retval elem: elemento in testa alla coda.
 */
qelem_t *pop_queue(queue_t *queue);

/** Crea e configura il socket AF_UNIX da utilizzare con 
    il processo visualizer.
    Riapre il file descriptor del client quando init
    è settato a 0.

    \param *sfd: puntatore al socket file descriptor del client.
    \param *sck_addr: puntatore all'indirizzo del socket.
    \param init: specifica se inizializzare la struct dell'indirizzo.

    \retval 0 se la procedura è andata a buon termine.
    \retval -1 se ci sono stati errori (errno settato).
*/
int build_client(int *sfd,
                 struct sockaddr_un *sck_addr,
                 short init);

/** Crea e configura il socket AF_UNIX da utilizzare con 
    il processo visualizer.

    \param *sfd: puntatore al socket file descriptor del server.
    \param *sck_addr: puntatore all'indirizzo del socket.

    \retval 0 se la procedura è andata a buon termine.
    \retval -1 se ci sono stati errori (errno settato).
*/
int build_server(int *sfd,
                 struct sockaddr_un *sck_addr);

#endif /* __WATOR__H */

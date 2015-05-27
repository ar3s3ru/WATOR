#include "utils.h"

#ifdef __UTILS_H__
#define __WATOR__

/* Nome di default del dumpfile */
#define DUMPF_DEF "visualizer.dump"

/** Struttura per i parametri di configurazione
    del processo.
 */
struct conf_param
{
    int workers;        /* Numero di workers */
    int chronons;       /* Chronon di aggiornamento */
    char *dumpfile;     /* Nome del dumpfile */
    char *watorfile;   /* Nome del file planet */
};

/** Struttura per il worker ID: contiene il tID del
    worker thread più il suo wID.
 */
typedef struct _worker_id
{
    pthread_t w_tID;
    int w_wID;
} wthread_t;

/** Struttura per l'aspetto multithread: dispatcher ID,
    worker IDs e collector ID.
 */
struct thread_ids
{
    pthread_t dispID;
    pthread_t collID;
    wthread_t *workIDs;
};

/** Gestisce il segnale SIGINT, mettendo in atto la
    terminazione gentile.

    \param pid: Process ID del processo visualizer
 */
static void sigint_handler(int pid);

/** Gestisce il segnale SIGTERM, mettendo in atto la
    terminazione gentile.

    \param pid: Process ID del processo visualizer
 */
static void sigterm_handler(int pid);

/** Gestisce il segnale SIGTERM, mandando richiesta di
    checkpointing al processo visualizer per il prossimo
    chronon.

    \param pid: Process ID del processo visualizer
 */
static void sigusr1_handler(int pid);

static void *dispatcherFoo();

/** Esegue la routine del thread collector.
    Avvia dunque la gestione del client-server side.

    \param bufarg: indirizzo del buffer.
 */
static void *collectorFoo(void *bufarg);

/** Esegue la routine del thread worker.
    
    \param arg: puntatore al valore del worker ID.
 */
static void *workerFoo(void *arg);

/** Funzione di cleanup all'invocazione della exit() */
static void exit_cleaner();

/** Funzione che gestisce il parsing del flag "-n".

    \param arg: argomento del flag.
    \param params: puntatore alla struttura dei parametri.

    \retval 0 se la procedura è andata a buon fine.
    \retval -1 in caso di errori (errno settato).
 */
static int parsing_worker(char *arg, struct conf_param *params);

/** Funzione che gestisce il parsing del flag "-v".

    \param arg: argomento del flag.
    \param params: puntatore alla struttura dei parametri.

    \retval 0 se la procedura è andata a buon fine.
    \retval -1 in caso di errori (errno settato).
 */
static int parsing_chronon(char *arg, struct conf_param *params);

/** Funzione che gestisce il parsing del flag "-f".

    \param arg: argomento del flag.
    \param params: puntatore alla struttura dei parametri.

    \retval 0 se la procedura è andata a buon fine.
    \retval -1 in caso di errori (errno settato).
 */
static int parsing_dumpfile(char *arg, struct conf_param *params);

/** Funzione che gestisce il parsing del file wator.

    \param arg: argomento del flag.
    \param params: puntatore alla struttura dei parametri.

    \retval 0 se la procedura è andata a buon fine.
    \retval -1 in caso di errori (errno settato).
 */
static int parsing_watorfile(char *arg, struct conf_param *params);

/** Funzione che parsa gli argomenti del programma,
    impostando correttamente i parametri necessari e
    controllando eventuali errori.

    \param argc: numero di argomenti del programma.
    \param argv: vettore degli argomenti del programma.
    \param params: puntatore alla struttura dei parametri.

    \retval 0 se la procedura è andata a buon fine.
    \retval -1 in caso di errori (errno settato).
 */
static int parser(int argc, char *argv[], struct conf_param *params);

/** Crea i thread workers.
    Chiama le pthread_create() e gestisce gli errori
    uscendo eventualmente dal processo.

    \param tIDs: struttura dei thread IDs.

    \retval 0 in caso di successo.
    \retval -1 in caso di errore (errno settato).
 */
static int workers_creation(struct thread_ids *tIDs);

/** Crea i thread dispatcher e collector.
    Chiama le pthread_create() e gestisce gli errori
    uscendo eventualmente dal processo.

    \param tIDs: struttura dei thread IDs.
    \param buffer: puntatore al buffer da dare al collector.

    \retval 0 in caso di successo.
    \retval -1 in caso di errore (errno settato).
 */
static int thread_creation(struct thread_ids *tIDs, char *buffer);

static int produce_buffer(wator_t *WATOR, char *buffer);

#endif /* __UTILS_H__ */
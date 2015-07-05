/** 
    \file wator_process.h
    \author Danilo Cianfrone, matricola 501292

    Il programma qui presente è, in ogni sua parte, opera originale dell'autore.
    Danilo Cianfrone.
 */

#include "utils.h"

#ifdef __UTILS_H__
#define __WATOR__

/* Nome di default del dumpfile */
#define DUMPF_DEF "wator.check"

/** Struttura per i parametri di configurazione
    del processo.
 */
struct conf_param
{
    int   workers;     /* Numero di workers */
    int   chronons;    /* Chronon di aggiornamento */
    char *dumpfile;    /* Nome del dumpfile */
    char *watorfile;   /* Nome del file planet */
};

/** Struttura per il worker ID: contiene il tID del
    worker thread più il suo wID.
 */
typedef struct _worker_id
{
    pthread_t w_tID;
    int       w_wID;
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

/** Il segnale SIGUSR2 viene inviato dal processo visualizer
    per comunicare di essere uscito con successo.
 */
static void sigusr2_handler();

/** Inizializza i parametri necessari per la generazione delle task
    e l'utilizzo di esse da parte dei worker.
 */
inline static void setting_tasks();

/** La funzione produce le tasks per la simulazione da dare ai worker.
    Viene invocata dal thread dispatcher.

    \param WATOR: puntatore alla struttura WATOR.
    \param workers: numero di workers.
    \param divider: radice quadrata del numero di task da generare.

    \retval NULL in caso di errore.
    \retval elems in caso di successo.
 */
static rect_t *produce_tasks(wator_t *WATOR, int workers, int divider);

/** La funzione aggiorna la struttura WATOR assicurando thread safety.
    Lavora su una porzione della struttura (descritta dall'elemento workset)
    utilizzando il mutex specifico della propria porzione e il mutex per
    le zone critiche di aggiornamento.

    \param WATOR: puntatore alla struttura WATOR.
    \param workset: puntatore all'elemento che descrive la porzione
                    della struttura da aggiornare.
    \param mtx_elem: puntatore al mutex del workset.
    \param critic_mtx: puntatore al mutex per le zone critiche.

    \retval -1 in caso di errore (errno settato).
    \retval 0 in caso di successo.
 */
static int update_wator_multithread(wator_t *WATOR, qelem_t *workset,
    pthread_mutex_t *mtx_elem, pthread_mutex_t *critic_mtx);

/** Esegue la routine del thread dispatcher.
    È responsabile della creazione dei thread worker,
    e della generazione delle task da computare.
 */
static void *disp_routine();

/** Esegue la routine del thread worker.
    Riceve le task dal thread worker e le utilizza
    computando gli n-chronon stabiliti dalla simulazione.
    
    \param arg: puntatore al valore del worker ID.
 */
static void *work_routine(void *arg);

/** Esegue la routine del thread collector.
    Avvia dunque la gestione del client-server side.

    \param bufarg: indirizzo del buffer.
 */
static void *coll_routine(void *bufarg);

/** Funzione di cleanup all'invocazione della exit().
    Il programma tiene traccia delle risorse allocate
    attraverso un numero definito di "step".

    La exit_cleaner() esegue il cleanup in relazione
    alle risorse generate durante l'esecuzione del 
    processo.
 */
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

/** A partire dal pianeta WATOR, produce il buffer
    che il thread collector manda tramite socket al
    processo visualizer.

    \param WATOR: puntatore alla struttura WATOR.
    \param buffer: puntatore al buffer.

    \retval 0: produzione con successo.
    \retval -1: errore (errno settato).
 */
static int produce_buffer(wator_t *WATOR, char *buffer);

#endif /* __UTILS_H__ */
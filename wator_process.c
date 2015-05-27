#include "wator_process.h"

#ifdef __WATOR__

extern int errno;

static int pid;                /* Process ID per la fork del visualizer */
static int wpid;               /* Process ID del processo wator */ 
static int sck_fd;             /* Socket file descriptor */
static int can_send     = 0;
static int step         = 0;   /* Descrive gli step del processo */
static int workers_done = 0;   /* Numero di workers che hanno 
                                  terminato la computazione */
static struct thread_ids tIDs; /* Threads IDs */

volatile sig_atomic_t looping = 1;
volatile sig_atomic_t dumping = 0;
volatile sig_atomic_t sig     = 0;
volatile sig_atomic_t has_quit_vis = 0; 

char *buffer   = NULL; /* Buffer per la read */
wator_t *Wator = NULL; /* Struttura WATOR */

queue_t q_task           = { NULL, NULL, 0 };    /* Coda condivisa */
struct conf_param params = {-1, -1, NULL, NULL}; /* Parametri processo */

/* Mutex */
static pthread_mutex_t mtx_wator   = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_queue   = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_send    = PTHREAD_MUTEX_INITIALIZER;
/* Condition variables */
static pthread_cond_t cond_workers  = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_queue    = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_send     = PTHREAD_COND_INITIALIZER;


static void sigint_handler(int pid)
{
    /* Interrompi il loop e invia SIGINT al visualizer */
    looping = 0;
    sig = SIGINT;
}

static void sigterm_handler(int pid)
{
    /* Interrompi il loop e invia SIGTERM al visualizer */
    looping = 0;
    sig = SIGTERM;
}

static void sigusr1_handler(int pid)
{
    /* Invia SIGUSR1 al processo visualizer */
    dumping = 1;
}

static void sigusr2_handler(int pid)
{
    /* Invia SIGUSR1 al processo visualizer */
    has_quit_vis = 1;
}


static void *dispatcherFoo()
{
    int i;
    rect_t *elems;

    /*fprintf(stderr, "Dispatcher thread created.\n");
    fflush(stderr);*/

    if ( workers_creation(&tIDs) == -1 )
    {
        perror("Error creating workers");
        pthread_exit((void *) 1);
    }

    /* Passaggio allo STEP 4, completato */
    ++step;

    while (looping)
    {
        if ( (elems = (rect_t *) malloc(sizeof(rect_t) * params.workers)) 
                == NULL )
        {
            perror("Error allocating elements");
            pthread_exit((void *) 1);
        }

        for (i = 0; i < params.workers; ++i)
        {
            elems[i].p1.x = 0;
            elems[i].p1.y = 0;
            elems[i].p2.x = 1;
            elems[i].p2.y = 1;
        }

        /*fprintf(stderr, "[D] Elements created.\n");
        fflush(stderr);*/

        pthread_mutex_lock(&mtx_queue);
    
        /*fprintf(stderr, "[D] Lock queue granted.\n");
        fflush(stderr);*/

        /* Elaborazione in mutua esclusione */
        for (i = 0; i < params.workers; ++i)
            push_queue(elems[i], &q_task);

        /*fprintf(stderr, "[D] Elements pushed.\n");
        fflush(stderr);*/

        /* Risveglia tutti i thread bloccati sulla queue */
        pthread_cond_broadcast(&cond_queue);
        /*fprintf(stderr, "[D] Broadcast workers.\n");
        fflush(stderr);*/

        /* Attendi che tutti i worker abbiano terminato */
        while (!isempty_queue(&q_task) && workers_done != params.workers)
        {
            /*if (looping == 0) break;*/
            /*fprintf(stderr, "[D] Waiting on cond workers_done.\n");
            fflush(stderr);*/
            pthread_cond_wait(&cond_workers, &mtx_queue);
        }
        workers_done = 0;
        /* Termine elaborazione in mutua esclusione */
        pthread_mutex_unlock(&mtx_queue);

        /*fprintf(stderr, "[D] Unlocked queue.\n");
        fflush(stderr);*/
       
        pthread_mutex_lock(&mtx_send);
        can_send = 1;
        pthread_cond_signal(&cond_send);
        pthread_mutex_unlock(&mtx_send);

        if (looping == 0)
            pthread_cond_broadcast(&cond_queue);

        /*fprintf(stderr, "[D] Looping: %d\n", looping);
        fflush(stderr);*/
    }

    for (i = 0; i < params.workers; ++i)
        pthread_join(tIDs.workIDs[i].w_tID, NULL);

    pthread_exit((void *) 0);
}

static void *collectorFoo(void *bufarg)
{
    int cln_fd;
    char *buffer = (char *) bufarg;

    /*fprintf(stderr, "Collector thread created.\n");
    fflush(stderr);*/

    cln_fd = accept(sck_fd, NULL, 0);
    /* Invio le dimensioni del pianeta al visualizer */
    write(cln_fd, &(Wator->plan->nrow), sizeof(unsigned int));
    write(cln_fd, &(Wator->plan->ncol), sizeof(unsigned int));
    close(cln_fd);

    /*fprintf(stderr, "[C] Sizes sent.\n");
    fflush(stderr);*/

    while (looping)
    {
        pthread_mutex_lock(&mtx_send);

        while (!can_send)
        {
            /*fprintf(stderr, "[C] Waiting on cond1 send.\n");
            fflush(stderr);*/
            pthread_cond_wait(&cond_send, &mtx_send);
        }

        can_send = 0;
        pthread_mutex_unlock(&mtx_send);

        pthread_mutex_lock(&mtx_wator);
       /* fprintf(stderr, "[C] Lock WATOR granted.\n");
        fflush(stderr);*/

        if (produce_buffer(Wator, buffer) == -1)
        {
            perror("Error creating buffer");
            pthread_mutex_unlock(&mtx_wator);
            pthread_exit((void *) EINVAL);
        }
        pthread_mutex_unlock(&mtx_wator);

        /*fprintf(stderr, "[C] Unlocked and buffer produced.\n");
        fflush(stderr);*/

        /* Accetta la connessione del client */
        cln_fd = accept(sck_fd, NULL, 0);

        /* Utilizza il file descriptor del client
           per tutte le operazioni di comunicazione. */
        write(cln_fd, buffer, 
            (Wator->plan->nrow * Wator->plan->ncol));
        close(cln_fd);

        /*fprintf(stderr, "[C] Buffer written, sending SIGUSR2 to visualizer.\n");
        fflush(stderr);*/

        if (dumping)
        {
            kill(pid, SIGUSR1);
            dumping = 0;
        }
        else
            kill(pid, SIGUSR2);

        /*fprintf(stderr, "[C] Looping: %d\n", looping);
        fflush(stderr);*/
    }

    pthread_exit((void *) 0);
}

static void *workerFoo(void *arg)
{
    qelem_t *workset  = NULL;
    char filename[20] = "wator_worker_";
    int wID           = *((int *) arg);
    int chronons;
    int fd;

    sprintf(&filename[13], "%d", wID);

    /*fprintf(stderr, "[W%d] %s created.\n", wID, filename);
    fflush(stderr);*/

    /* Crea il file di check */
    if ( (fd = open(filename, O_RDWR|O_CREAT, 0666)) == -1 )
    {    
        /* File non creato correttamente */
        perror("Error creating wid file");
        pthread_exit((void *) &errno);
    }

    close(fd);

    while (looping)
    {
        pthread_mutex_lock(&mtx_queue);
        /*fprintf(stderr, "[W%d] Lock queue granted.\n", wID);
        fflush(stderr);*/
        /* Elaborazione in mutua esclusione */
        while (isempty_queue(&q_task) && looping == 1)
        {
            /*fprintf(stderr, "[W%d] Waiting on cond1 queue.\n", wID);
            fflush(stderr);*/
            pthread_cond_wait(&cond_queue, &mtx_queue);
        }

        workset = pop_queue(&q_task);
        /*fprintf(stderr, "[W%d] Element got, unlocking.\n", wID);
        fflush(stderr);*/
        /* Termine elaborazione in mutua esclusione */
        pthread_mutex_unlock(&mtx_queue);

        chronons = 0;
        pthread_mutex_lock(&mtx_wator);
        /* Lavora sull'elemento workset */
        free(workset);
        /*fprintf(stderr, "[W%d] Lock WATOR granted.\n", wID);
        fflush(stderr);*/
        while (chronons < params.chronons)
        {
            ++chronons;
        }

        pthread_mutex_unlock(&mtx_wator);

        /*fprintf(stderr, "[W%d] Work terminated and unlocked WATOR.\n", wID);
        fflush(stderr);*/

        pthread_mutex_lock(&mtx_queue);
        ++workers_done;
        /* Comunica al dispatcher di aver terminato la computazione */
        if (workers_done == params.workers || isempty_queue(&q_task))
            pthread_cond_signal(&cond_workers);
        pthread_mutex_unlock(&mtx_queue);

        /*fprintf(stderr, "[W%d] Looping: %d\n", wID, looping);
        fflush(stderr);*/
    }

    pthread_exit((void *) 0);
}

static void exit_cleaner()
{

    /* STEP 1: Libera la memoria allocata dinamicamente */
    if (step > 0)
    {
        if (strcmp(DUMPF_DEF, params.dumpfile) != 0)
            free(params.dumpfile);
        free(params.watorfile);
    }

    /* STEP 2: Dealloca la struttura WATOR */
    if (step > 1)
    {
        free_wator(Wator);
    }

    /* STEP 3: Chiudi i file descriptors aperti */
    if (step > 2)
    {
        close(sck_fd);
        /* Ripulisci il socket all'uscita */
        unlink(SOCKNAME);
    }

    /* STEP 4: Dealloca l'array dei worker thread */
    if (step > 3)
    {
        free(buffer);
        free(tIDs.workIDs);
        /* Ripulisco i file temporanei del worker */
        system("rm wator_worker_*");
    }
}

static int parsing_worker(char *arg, struct conf_param *params)
{
    int temp;

    if (arg == NULL || params == NULL)
        /* Parametri di funzione errati */
        goto param_error;

    sscanf(arg, "%d", &temp);

    if (temp < 1)
        /* Parametro errato */
        goto param_error;

    params->workers = temp;
    return 0;

    param_error:
        errno = EINVAL;
        return -1;
}

static int parsing_chronon(char *arg, struct conf_param *params)
{
    int temp;

    if (arg == NULL || params == NULL)
        /* Parametro di funzione errati */
        goto param_error;

    sscanf(arg, "%d", &temp);

    if (temp < 1)
        /* Parametro errato */
        goto param_error;

    params->chronons = temp;
    return 0;

    param_error:
        errno = EINVAL;
        return -1;
}

static int parsing_dumpfile(char *arg, struct conf_param *params)
{
    int lenght;

    if (arg == NULL    || 
        params == NULL || params->dumpfile != NULL)
    {    
        /** Parametri di funzione errati.
            dumpfile dovrebbe essere inizializzato a NULL.
         */
        errno = EINVAL;
        return -1;
    }

    lenght = strlen(arg);

    if ( (params->dumpfile = (char *) malloc(sizeof(char) * (lenght + 1) )) == NULL )
        /* Errore di allocazione, errno già settato */
        return -1;

    strcpy(params->dumpfile, arg);
    return 0;
}

static int parsing_watorfile(char *arg, struct conf_param *params)
{
    int lenght;

    if (arg == NULL    || 
        params == NULL || params->watorfile != NULL)
    {    
        /** Parametri di funzione errati.
            watorfile dovrebbe essere inizializzato a NULL.
         */
        errno = EINVAL;
        return -1;
    }

    lenght = strlen(arg);

    if ( (params->watorfile = (char *) malloc(sizeof(char) * (lenght + 1) )) == NULL )
        /* Errore di allocazione, errno già settato */
        return -1;

    strcpy(params->watorfile, arg);
    return 0;
}

static int parser(int argc, char *argv[], struct conf_param *params)
{
    int i;

    if (argc < 1 || argv == NULL || params == NULL)
    {
        /* Parametri di funzione errati */
        errno = EINVAL;
        return -1;
    }

    for (i = 0; i < argc; ++i)
    {
        if ( strcmp(argv[i], "-n") == 0 )
        {
            /* Gestisci il flag "-n" */
            err_check_min1(parsing_worker(argv[++i], params), -1);
        }
        else if ( strcmp(argv[i], "-v") == 0 )
        {
            /* Gestisci il flag "-v" */
            err_check_min1(parsing_chronon(argv[++i], params), -1);
        }
        else if ( strcmp(argv[i], "-f") == 0 )
        {
            /* Gestisci il flag "-f" */
            err_check_min1(parsing_dumpfile(argv[++i], params), -1);
        }
        else
        {
            /* Gestisci il caso generale, supposto "file" */
            err_check_min1(parsing_watorfile(argv[++i], params), -1);
        }
    }

    return 0;
}

static int workers_creation(struct thread_ids *tIDs)
{
    int i;

    if (tIDs == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    /* Allocazione memoria per workers thread array */
    if ( (tIDs->workIDs = 
            (wthread_t *) malloc(sizeof(wthread_t) * params.workers)) == NULL )
    {
        perror("Error allocating wID array");
        return -1;
    }

    for (i = 0; i < params.workers; ++i)
    {
        /* Creazione worker threads */
        tIDs->workIDs[i].w_wID = i;
        errno = pthread_create(&(tIDs->workIDs[i].w_tID), NULL, 
            workerFoo, (void *) &(tIDs->workIDs[i].w_wID));

        if (errno != 0)
        {
            perror("Error creating worker threads");
            return -1;
        }
    }

    return 0;
}

static int thread_creation(struct thread_ids *tIDs, char *buffer)
{
    if (tIDs == NULL || buffer == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    /* Creazione dispatcher thread */
    errno = pthread_create(&(tIDs->dispID), NULL, dispatcherFoo, NULL);
    if (errno != 0)
    {
        perror("Error creating dispatcher thread");
        return -1;
    }

    /* Creazione collector thread */
    errno = pthread_create(&(tIDs->collID), NULL, 
        collectorFoo, (void *) buffer);
    if (errno != 0)
    {
        perror("Error creating collector thread");
        return -1;
    }

    return 0;
}

static int produce_buffer(wator_t *WATOR, char *buffer)
{
    int i, j, k;

    if (WATOR == NULL || WATOR->plan == NULL
        || WATOR->plan->nrow < 1 || WATOR->plan->ncol < 1)
    {
        errno = EINVAL;
        return -1;
    }

    for (i = 0, k = 0; i < WATOR->plan->nrow; ++i)
    {
        for (j = 0; j < WATOR->plan->ncol; ++j, ++k)
        {
            assert(k < (WATOR->plan->nrow * WATOR->plan->ncol));
            buffer[k] = cell_to_char(WATOR->plan->w[i][j]);
        }
    }

    return 0;
}

int main(int argc, char *argv[], char *envp[])
{
    DIR *dir;   /* Per la cartella ./tmp */

    struct sockaddr_un sck_addr; /* Indirizzo del socket */
    struct sigaction signals;    /* Signal bitmaps */
    struct stat check_conf;      /* Per il check su "wator.conf" */
    sigset_t set;

    wpid = getpid();

    /* Ripulisci il socket all'uscita */
    unlink(SOCKNAME);

    if (argc < 2 || argc > 8 || (argc % 2) != 0)
    {
        /* Numero di parametri errati, esci */
        /** (argc % 2) mi assicura che il numero di 
            parametri è effettivamente corretto */
        exit(EXIT_FAILURE);
    }

    /* Check esistenza di "wator.conf" */
    if (stat("wator.conf", &check_conf) == -1 && errno == ENOENT)
    {
        perror("wator.conf doesn't exists");
        exit(EXIT_FAILURE);
    }

    if (atexit(exit_cleaner) != 0)
        /* Errore nel setting della cleanup function */
        exit(errno);

    /** Mascheriamo i segnali fino alla fine dell'installazione
        dei gestori. 
     */
    err_check(sigfillset(&set),
              -1,
              "Error filling signal mask");

    err_check(pthread_sigmask(SIG_SETMASK, &set, NULL),
              -1,
              "Error setting signal mask (1)");

    /* Setto la signal bitmap a zero */
    memset(&signals, 0, sizeof(signals)); 

    /* Installazione gestore per SIGINT con check sull'errore */
    signals.sa_handler = sigint_handler;
    err_check(sigaction(SIGINT, &signals, NULL), 
              -1,
              "Installation SIGINT handler failed");

    /* Installazione gestore per SIGTERM con check sull'errore */
    signals.sa_handler = sigterm_handler;
    err_check(sigaction(SIGTERM, &signals, NULL), 
              -1,
              "Installation SIGTERM handler failed");

    /* Installazione gestore per SIGUSR1 con check sull'errore */
    signals.sa_handler = sigusr1_handler;
    err_check(sigaction(SIGUSR1, &signals, NULL), 
              -1,
              "Installation SIGUSR1 handler failed");

    /* Installazione gestore per SIGUSR2 con check sull'errore */
    signals.sa_handler = sigusr2_handler;
    err_check(sigaction(SIGUSR2, &signals, NULL), 
              -1,
              "Installation SIGUSR2 handler failed");

    /** Resettiamo la maschera dei gestori */ 
    err_check(sigemptyset(&set),
              -1,
              "Error emptying signal mask");

    err_check(pthread_sigmask(SIG_SETMASK, &set, NULL),
              -1,
              "Error setting signal mask (2)");


    /* Avvio del parser dei parametri del processo */
    err_check(parser(argc, argv, &params),
              -1,
              "Parsing args failed");

    /* watorfile non esiste; tratta errore. */
    if (params.watorfile == NULL || 
       (stat(params.watorfile, &check_conf) == -1 &&
        errno == ENOENT))
    {
        perror("Invalid WATOR file specified");
        exit(EXIT_FAILURE);
    }

    if (params.workers == -1) params.workers = NWORK_DEF;
    if (params.chronons == -1) params.chronons = CHRON_DEF;
    if (params.dumpfile == NULL) params.dumpfile = DUMPF_DEF;

    /* Passaggio allo STEP 1, completato. */
    ++step;

    /* Carica la struttura WATOR da watorfile. */
    err_check((Wator = new_wator(params.watorfile)),
               NULL,
               "Error loading WATOR file");

    /* Passaggio allo STEP 2, completato */
    ++step;

    /* Crea la cartella temporanea /tmp */
    if ( (dir = opendir("./tmp")) != NULL )
        /* La cartella esiste */
        closedir(dir);
    else if (errno == ENOENT)
    {    
        /* La cartella non esiste, creala.
            In caso di errore, esci con errno. 
         */
        err_check(mkdir("./tmp", 0775), 
                  -1,
                  "Error creating /tmp folder");
    }
    else
        /* La opendir() fallisce per qualche altro motivo */
        exit(errno);
    
    /** Creo e configuro il socket.
        Tratta l'errore in caso di ritorno -1.
     */
    err_check(build_server(&sck_fd, &sck_addr), 
              -1,
              "Error building server");

    /* Passaggio allo STEP 3, completato. */
    ++step;

    /* Forka il figlio e avvia visualizer */  
    if ( (pid = fork()) == 0 )
    {
        execl("./visualizer", "./visualizer", params.dumpfile, NULL);
        /* Errore: non deve ritornare */
        exit(EXIT_FAILURE);
    }
    else if (pid == -1)
        /* Errore sulla fork */ 
        exit(errno);

    /* Allocazione spazio per il buffer */
    err_check((buffer = (char *) malloc(sizeof(char) * 
        (Wator->plan->nrow * Wator->plan->ncol))),
               NULL,
               "Error allocating buffer");

    /* Creazione thread */
    if (thread_creation(&tIDs, buffer) == -1)
        /* Errore creazione thread */
        exit(errno);

    pthread_join(tIDs.collID, NULL);
    pthread_join(tIDs.dispID, NULL);

    /** Uccide visualizer con segnale sig e aspetta 1 secondo
        affinchè il segnale venga gestito */
    while(!has_quit_vis)
    {
        kill(pid, sig);
        sleep(1);
    }
    
    exit(EXIT_SUCCESS);
}

#endif /* __WATOR__ */
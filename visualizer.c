#include "visualizer.h"

#ifdef __VISUALIZER__

extern int errno;
static int sck_fd;          /* Socket file descriptor */
static int step = 0;        /* STEPS counter */
static char *buffer;        /* Buffer per la write */
static FILE *dmp_fs = NULL; /* Dump file stream */

volatile sig_atomic_t dumping     = 0;
volatile sig_atomic_t looping     = 1;
volatile sig_atomic_t can_receive = 0;

static void sigint_handler()
{
    /* TODO */
    /*can_receive = 1;*/
    looping = 0;
    /*dumping = 1;*/
}

static void sigterm_handler()
{
    /* TODO */
    /*can_receive = 1;*/
    looping = 0;
    /*dumping = 1;*/
}

static void sigusr1_handler()
{
    can_receive = 1;
    dumping = 1;
}

static void sigusr2_handler()
{
    can_receive = 1;
}

static void exit_cleaner()
{
    /* STEP 1: Chiudi i file descriptor aperti */
    if (step > 0)
    {
        fprintf(stderr, "[VIS] Step = %d, doing STEP 1.\n", step);
        fflush(stderr);
        fclose(dmp_fs);
    }

    /* STEP 2: Libera il buffer della socket */
    if (step > 1)
    {
        fprintf(stderr, "[VIS] Step = %d, doing STEP 2.\n", step);
        fflush(stderr);
        free(buffer);
    }

    kill(getppid(), SIGUSR2);
}

inline static int update_screen(FILE *stream, char *buffer, 
    unsigned int nrow, unsigned int ncol)
{
    int i, j;

    if (stream == NULL || buffer == NULL || 
        nrow <= 0 || ncol <= 0)
    {
        errno = EINVAL;
        return -1;
    }

    for (i = 0, j = 0; i < (nrow * ncol); ++i, ++j)
    {
        /* Termine colonna, vai a capo */
        if (j == ncol - 1)
        {    
            fprintf(stream, "%c\n", buffer[i]);
            j = -1;
        }
        else
            fprintf(stream, "%c ", buffer[i]);
    }

    fflush(stream);
    return 0;
}

int main(int argc, char *argv[], char *envp[])
{   
    unsigned int nrow;    /* Numero di righe del WATOR */
    unsigned int ncol;    /* Numero di colonne del WATOR */
    long rewrite_off = 0; /* Offset dal quale riprendere la scrittura su dump */
    /*long iteration   = 0;*/

    struct sockaddr_un sck_addr; /* Indirizzo della socket */
    struct sigaction signals;     /* Signal bitmap */
    sigset_t set;

    if (argc != 2)
    {
        perror("Invalid arguments");
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

    /* Apro il file per il dump */
    err_check((dmp_fs = fopen(argv[1], "w+")),
               NULL,
               "Error opening dumpfile");

    /* Creo e configuro il socket.
       Inizializzazione attivata (init = 1).
       In caso di errore (retval -1) esci.
     */
    err_check(build_client(&sck_fd, &sck_addr, 1), 
              -1,
              "Error building client");

    /* STEP 1, completato. */
    ++step;

    read(sck_fd, &nrow, sizeof(unsigned int));
    read(sck_fd, &ncol, sizeof(unsigned int));
    close(sck_fd);

    assert(nrow > 0 && ncol > 0);

    fprintf(dmp_fs, "%d\n%d\n", nrow, ncol);
    fflush(dmp_fs);
    rewrite_off = ftell(dmp_fs);

    /* Alloca spazio per il buffer del socket */
    err_check((buffer = (char *) malloc(sizeof(char) * (nrow * ncol))),
               NULL,
               "Error creating buffer");

    /* STEP 2, completato. */
    ++step;

    while (looping)
    {
        if (build_client(&sck_fd, &sck_addr, 0) == -1)
        {
            perror("Error creating client");
            exit(errno);
        }

        fprintf(stderr, "[VIS] Waiting for signal.\n");
        fflush(stderr);

        while (!can_receive)
        {
            if (looping == 0)
            {
                fprintf(stderr, "[VIS] Looping = 0, force break\n");
                fflush(stderr);
                can_receive = 1;
            }
            sleep(1);
        }

        fprintf(stderr, "[VIS] Signal received, getting data.\n");
        fflush(stderr);
        can_receive = 0;
        
        read(sck_fd, buffer, (nrow * ncol));
        close(sck_fd);

        if (!dumping)
        {
            fprintf(stderr, "[VIS] Printing on screen.\n");
            fflush(stderr);
            /* Mostra il pianeta a schermo */
            system("clear");    /* Invoca la clear della shell */
            update_screen(stdout, buffer, nrow, ncol);
            /*fprintf(stdout, "Iteration: %ld\n", iteration);
            fflush(stdout);
            ++iteration;*/
        }
        else
        {
            fprintf(stderr, "[VIS] Dumping on %s.\n", argv[1]);
            fflush(stderr);
            /* Scrivi il pianeta sul file */
            fseek(dmp_fs, rewrite_off, SEEK_SET);
            update_screen(dmp_fs, buffer, nrow, ncol);
            dumping = 0;
        }

        fprintf(stderr, "[VIS] Sleeping...\n");
        fflush(stderr);
        sleep(1);
        fprintf(stderr, "[VIS] Looping = %d\n", looping);
        fflush(stderr);
    }

    fprintf(stderr, "[VIS] Exiting.\n");
    fflush(stderr);

    exit(EXIT_SUCCESS);
}

#endif /* __VISUALIZER__ */
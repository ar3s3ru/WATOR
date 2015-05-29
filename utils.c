/** 
    \file utils.c
    \author Danilo Cianfrone, matricola 501292

    Il programma qui presente è, in ogni sua parte, opera originale dell'autore.
    Danilo Cianfrone.
 */
    
#include "utils.h"

#ifdef __UTILS_H__

inline int isempty_queue(queue_t *queue)
{
    if (queue == NULL)
    {
        /* Argomento errato */
        errno = EINVAL;
        return -1;
    }

    return !(queue->elem);
}

int push_queue(rect_t elem, queue_t *queue)
{
    qelem_t *qelem;

    if (queue == NULL)
    {
        /* Parametri errati */
        errno = EINVAL;
        return -1;
    }

    if ( (qelem = (qelem_t *) malloc(sizeof(qelem_t))) == NULL )
        return -1;

    qelem->info = elem;
    qelem->next = NULL;

    if (isempty_queue(queue))
    {
        /* Coda vuota */
        queue->head = qelem;
        queue->tail = qelem;
    }
    else
    {
        queue->tail->next = qelem;
        queue->tail = qelem;
        qelem->next = NULL;
    }

    ++(queue->elem);

    return 0;
}

qelem_t *pop_queue(queue_t *queue)
{
    qelem_t *temp;

    if (queue == NULL || isempty_queue(queue))
    {
        /* Argomenti non validi */
        errno = EINVAL;
        return NULL;
    }

    temp = queue->head;
    queue->head = queue->head->next;
    temp->next = NULL;

    --(queue->elem);

    return temp;
}

int build_client(int *sfd,
                 struct sockaddr_un *sck_addr,
                 short init)
{
    if (sfd == NULL || sck_addr == NULL)
    {
        /* Argomenti errati */
        errno = EINVAL;
        return -1;
    }

    if (init == 1)
    {
        /* Inizializzazione della struttura sockaddr */
        sck_addr->sun_family = AF_UNIX;
        strncpy(sck_addr->sun_path, SOCKNAME, UNIX_PATH_MAX);
    }

    /* Creazione socket */
    err_check_min1( (*sfd = socket(AF_UNIX, SOCK_STREAM, 0)), -1); 

    /* Aspetta che la connessione sia stata effettivamente stabilita */
    while ( connect(*sfd, (struct sockaddr *) sck_addr, sizeof(*sck_addr)) == -1 )
    {
        if (errno != ENOENT)
            /* Errore (diverso dalla non-esistenza della socket) */
            return -1;
    }

    return 0;
}

int build_server(int *sfd,
                 struct sockaddr_un *sck_addr)
{
    if (sfd == NULL || sck_addr == NULL)
    {
        /* Argomenti errati */
        errno = EINVAL;
        return -1;
    }

    sck_addr->sun_family = AF_UNIX;
    strncpy(sck_addr->sun_path, SOCKNAME, UNIX_PATH_MAX);

    /* Creazione socket */
    err_check_min1( (*sfd = socket(AF_UNIX, SOCK_STREAM, 0)), 
                    -1); 

    /* Socket binding */
    err_check_min1( bind(*sfd, (struct sockaddr *) sck_addr, sizeof(*sck_addr)),
                    -1);

    /* Le connessioni in coda sono al massimo una, dal visualizer */
    err_check_min1( listen(*sfd, 1),
                    -1);

    /* Apertura del socket avvenuta con successo */
    return 0; 
}

#endif /* __UTILS_H__ */
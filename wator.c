/** 
    \file wator.c
    \author Danilo Cianfrone, matricola 501292

    Il programma qui presente è, in ogni sua parte, opera originale dell'autore.
    Danilo Cianfrone.
*/

#include "wator.h"

#ifdef __WATOR__H

/* Variabili globali */
static char **array = NULL;
extern int errno;

char cell_to_char(cell_t a)
{
    /* Consiste di un semplice "switch" per distinguere
       i vari casi. 
     */
    switch(a)
    {
        case WATER:
            return 'W';
        case SHARK:
            return 'S';
        case FISH:
            return 'F';
        default:
            return '?';
    }
}

int char_to_cell(char c)
{
    /* Come sopra. */
    switch (c)
    {
        case 'W':
            return WATER;
        case 'S':
            return SHARK;
        case 'F':
            return FISH;
        default:
            return -1;
    }
}

planet_t *new_planet(unsigned int nrow, unsigned int ncol)
{
    size_t i, j; /* Per iterare nell'allocazione delle righe */
    planet_t *result;

    if (nrow == 0 || ncol == 0)
    {
        errno_set(EINVAL, NULL);
    }

    /* Allocazione di "result", se fallita esci restituendo NULL */
    if ((result = (planet_t *) malloc(sizeof(planet_t))) == NULL)
    {
        /* errno è già settato */
        return NULL;
    }

    result->nrow = nrow;
    result->ncol = ncol;

    /* Array multidimensionali: allocazione puntatori a righe */
    result->w     = (cell_t **) malloc(sizeof(cell_t *) * nrow);
    result->btime = (int **)    malloc(sizeof(int *) * nrow);
    result->dtime = (int **)    malloc(sizeof(int *) * nrow);

    if (result->w     == NULL ||
        result->btime == NULL || result->dtime == NULL)
    {
        /* Errori di allocazione: restituisci NULL, errno già settato. */
        free(result);
        return NULL;
    }

    /* Array multidimensionali: allocazione righe */
    /* Le righe avranno un numero di elementi pari al numero
       di colonne della matrice. 
     */
    for (i = 0; i < nrow; ++i)
    {
        result->w[i]     = (cell_t *) malloc(sizeof(cell_t) * ncol);
        result->btime[i] = (int *)    malloc(sizeof(int) * ncol);
        result->dtime[i] = (int *)    malloc(sizeof(int) * ncol);

        if (result->w[i]     == NULL ||
            result->btime[i] == NULL || result->dtime[i] == NULL)
        {
            /* Errori di allocazione memoria, errno già settato. */
            free_planet(result);
            return NULL;
        }
    }

    /* Inizializzazione matrici morte e nascite */
    for (i = 0; i < nrow; ++i)
    {
        for (j = 0; j < ncol; ++j)
        {
            result->w[i][j]     = WATER;
            result->btime[i][j] = 0;
            result->dtime[i][j] = 0;
        }
    }

    /* Restituisce puntatore al nuovo pianeta */
    return result;
}

void free_planet(planet_t *p)
{
    size_t i;

    if (p != NULL)
    {
        for (i = 0; i < p->nrow; ++i)
        {
            /* Dealloca tutte le righe della matrice. */
            free(p->w[i]);
            free(p->btime[i]);
            free(p->dtime[i]);
        }

        /* Dealloca tutti i puntatori alle righe della matrice. */
        free(p->w);
        free(p->btime);
        free(p->dtime);

        /* Dealloca il pianeta p */
        free(p);
    }
}

int print_planet(FILE *f, planet_t *p)
{
    size_t i, j;
    int check;

    /* Parametri attuali errati */
    if (f == NULL || p == NULL)
    {
        errno_set(EINVAL, -1);
    }

    /* Scrivi nrow e ncol su file, con gestione errore di scrittura */
    if (fprintf(f, "%d\n", p->nrow) < 0 || fprintf(f, "%d\n", p->ncol) < 0)
    {
        errno_set(EIO, -1);
    }

    for (i = 0; i < p->nrow; ++i)
    {
        for (j = 0; j < p->ncol; ++j)
        {
            /* Ultimo carattere di una riga, stampare newline */
            if (j == p->ncol - 1)
                check = fprintf(f, "%c\n", cell_to_char(p->w[i][j]));
            /* Stampa uno spazio affianco alla riga, non è l'ultimo
               carattere di una riga.
             */
            else
                check = fprintf(f, "%c ", cell_to_char(p->w[i][j]));

            /* Errore di scrittura, ritorna -1 */
            if (check < 0) 
            {
                errno_set(EIO, -1);
            }
        }
    }

    /* Scrittura completata con successo. */
    return 0;
}

planet_t *load_planet(FILE *f)
{
    planet_t *result = NULL;

    unsigned int nrow, ncol;
    size_t i = 0, j = 0;  /* Iteratori su matrici */
    char prev = 0;        /* 0 per spazio precedente, 1 per carattere */
    char temp;

    /* Il file è NULL, parametro errato */
    if (f == NULL)
    {
        errno_set(EINVAL, NULL);
    }

    /* Eventuale errore nell'I/O */
    if (fscanf(f, "%u\n%u\n", &nrow, &ncol) <= 0)
    {
        errno_set(EIO, NULL);
    }

    result = new_planet(nrow, ncol);

    while ( (temp = fgetc(f)) != EOF && i < nrow )
    {
        switch (temp)
        {
            case ' ':
                /* Errore */
                if (prev == 0)
                { 
                    error_handle(handle_load_planet, NULL);
                }

                prev = 0;
                ++j;
                break;

            case '\n':
                /* Errore */
                if (prev == 0)
                {
                    error_handle(handle_load_planet, NULL);
                }

                prev = 0;
                j = ((i != nrow - 1) ? 0 : j + 1);
                ++i;
                break;

            default:
                /* Errore */
                if (prev == 1)
                {
                    error_handle(handle_load_planet, NULL);
                }

                result->w[i][j] = char_to_cell(temp);
                prev = 1;
                break;
        }
    }

    assert( (i == nrow) && (j == ncol) );

    /* Creazione con successo, ritorna la struttura */
    return result;
}

wator_t *new_wator(char *fileplan)
{
    FILE *conf, *plan;
    int i;

    wator_t *new_w;
    char temp[20];

    /* Stringa del file pianeta errata */
    if (fileplan == NULL)
    {
        errno_set(EINVAL, NULL);
    }

    /* errno è settato da open */
    if ((conf = fopen(CONFIGURATION_FILE, "r")) == NULL || 
        (plan = fopen(fileplan, "r"))           == NULL) 
        return NULL;

    /* Inizializza il seed per la RNG */
    srand(time(NULL));

    /* Alloca spazio per il wator, errno settato in caso di errori */
    if (( new_w = (wator_t *) malloc(sizeof(wator_t)) ) == NULL)
    {
        error_handle(handle_new_wator, NULL);
    }

    /* Prendi i parametri dal file di configurazione e tratta
       eventuali errori.
       errno = EIO (Input/Output error). 
     */
    if ( fscanf(conf, "%s %d", temp, &(new_w->sd)) <= 0 ||
         fscanf(conf, "%s %d", temp, &(new_w->sb)) <= 0 ||
         fscanf(conf, "%s %d", temp, &(new_w->fb)) <= 0 )
    {
        free_wator(new_w);    /* Libera la struttura */
        handle_new_wator();   /* Chiudi i file */
        errno_set(EIO, NULL); /* Setta errno e ritorna NULL */
    }

    new_w->nf      = 0;
    new_w->ns      = 0;
    new_w->nwork   = 0;
    new_w->chronon = 0;

    /* errno già settato da load_planet */
    if ( (new_w->plan = load_planet(plan)) == NULL)
    {
        free_wator(new_w);
        error_handle(handle_new_wator, NULL);
    }

    /* Chiudi i file dopo la creazione */
    handle_new_wator();

    /* La struttura è stata creata con successo, crea anche l'array d'appoggio */
    create_array(new_w);

    /* Struttura creata con successo */
    return new_w;
}

void free_wator(wator_t *pw)
{
    int i;
    
    if (pw != NULL)
    {
        /* Dealloca l'array d'appoggio */
        free_array(pw);
        /* Dealloca il planet_t */
        free_planet(pw->plan);
        free(pw);
    }
}

int shark_rule1(wator_t *pw, int x, int y, int *k, int *l)
{
    point_t adiac[4];
    cell_t  ad_val[4];

    unsigned char i;
    char fish_c  = 0;
    char shark_c = 0;
    char water_c = 0;

    /* Errore, struttura WATOR è uguale a NULL */
    if (pw == NULL || k == NULL || l == NULL)
    {
        errno_set(EINVAL, -1);
    }

    /* Punto (x-1, y) */
    adiac[0].x = (x == 0 ? pw->plan->nrow - 1 : x - 1);
    adiac[0].y = y;

    /* Punto (x, y-1) */
    adiac[1].x = x;
    adiac[1].y = (y == 0 ? pw->plan->ncol - 1 : y - 1);

    /* Punto (x+1, y) */
    adiac[2].x = (x == pw->plan->nrow - 1 ? 0 : x + 1);
    adiac[2].y = y;

    /* Punto (x, y+1) */
    adiac[3].x = x;
    adiac[3].y = (y == pw->plan->ncol - 1 ? 0 : y + 1);

    /* Prendiamo i valori dei punti adiacenti */
    for (i = 0; i < 4; ++i)
    {
        ad_val[i] = pw->plan->w[adiac[i].x][adiac[i].y];

        switch (ad_val[i])
        {
            case WATER:
                ++water_c;
                break;

            case SHARK:
                ++shark_c;
                break;

            case FISH:
                ++fish_c;
                break;

            default:
                errno_set(EIO, -1);
        }
    }

    /* Lo squalo rimane fermo */
    if (shark_c == 4)
    {
        *k = x;
        *l = y;
         
        return STOP;
    }

    while (1)
    {
        i = rand() % 4;

        *k = adiac[i].x;
        *l = adiac[i].y;

        /* C'è almeno un pesce, quindi mangia */
        if (fish_c > 0 && ad_val[i] == FISH)
        {
            pw->plan->w[x][y]   = WATER;
            pw->plan->w[*k][*l] = SHARK;

            /* Lo squalo mangiando "si allontana" dalla morte */
            pw->plan->btime[*k][*l] = pw->plan->btime[x][y];
            pw->plan->dtime[*k][*l] = 0;

            /* Resetto i contatori delle posizioni precedenti */
            pw->plan->btime[x][y] = 0;
            pw->plan->dtime[x][y] = 0;

            return EAT;
        }

        /* Non c'è un pesce, quindi si muove */
        if (fish_c == 0 && ad_val[i] == WATER)
        {
            pw->plan->w[x][y]   = WATER;
            pw->plan->w[*k][*l] = SHARK;

            pw->plan->btime[*k][*l] = pw->plan->btime[x][y];
            pw->plan->dtime[*k][*l] = pw->plan->dtime[x][y];

            pw->plan->btime[x][y] = 0;
            pw->plan->dtime[x][y] = 0;
            
            return MOVE;
        }
    }
}

int shark_rule2(wator_t *pw, int x, int y, int *k, int *l)
{
    point_t adiac[4];
    cell_t  ad_val[4];

    unsigned char i;
    char born = 0;

    /* Errore, struttura WATOR è uguale a NULL */
    if (pw == NULL || k == NULL || l == NULL)
    {
        errno_set(EINVAL, -1);
    }

    /* Punto (x-1, y) */
    adiac[0].x = (x == 0 ? pw->plan->nrow - 1 : x - 1);
    adiac[0].y = y;

    /* Punto (x, y-1) */
    adiac[1].x = x;
    adiac[1].y = (y == 0 ? pw->plan->ncol - 1 : y - 1);

    /* Punto (x+1, y) */
    adiac[2].x = (x == pw->plan->nrow - 1 ? 0 : x + 1);
    adiac[2].y = y;

    /* Punto (x, y+1) */
    adiac[3].x = x;
    adiac[3].y = (y == pw->plan->ncol - 1 ? 0 : y + 1);

    /* Prendiamo i valori dei punti adiacenti */
    for (i = 0; i < 4; ++i)
        ad_val[i] = pw->plan->w[adiac[i].x][adiac[i].y];

    *k = x;
    *l = y;

    /* Caso di nascita nuovo squalo */
    if (pw->plan->btime[x][y] < pw->sb)
        ++(pw->plan->btime[x][y]);

    else
    {
        pw->plan->btime[x][y] = 0;

        for (i = 0; i < 4 && !born; ++i)
        {
            if (ad_val[i] == WATER)
            {
                *k = adiac[i].x;
                *l = adiac[i].y;

                pw->plan->w[*k][*l] = SHARK;
                born = 1;
            }
        }
    }

    /* Caso di morte e return value */
    /* Il contatore della morte dello squalo non ha ancora
       raggiunto la dimensione massima, quindi incrementa
     */
    if (pw->plan->dtime[x][y] < pw->sd)
    {
        ++(pw->plan->dtime[x][y]);
        return ALIVE;
    }
    /* Il contatore morte ha raggiunto la dimensione massima
       quindi lo squalo muore.
     */
    else
    {
        pw->plan->dtime[x][y] = 0;
        pw->plan->btime[x][y] = 0;
        pw->plan->w[x][y]     = WATER;
        return DEAD;
    }
}

int fish_rule3 (wator_t* pw, int x, int y, int *k, int* l)
{
    point_t adiac[4];
    cell_t  ad_val[4];

    unsigned char i;
    char fish_c  = 0;
    char shark_c = 0;
    char water_c = 0;

    /* Errore, struttura WATOR è uguale a NULL */
    if (pw == NULL || k == NULL || l == NULL)
    {
        errno_set(EINVAL, -1);
    }

    /* Punto (x-1, y) */
    adiac[0].x = (x == 0 ? pw->plan->nrow - 1 : x - 1);
    adiac[0].y = y;

    /* Punto (x, y-1) */
    adiac[1].x = x;
    adiac[1].y = (y == 0 ? pw->plan->ncol - 1 : y - 1);

    /* Punto (x+1, y) */
    adiac[2].x = (x == pw->plan->nrow - 1 ? 0 : x + 1);
    adiac[2].y = y;

    /* Punto (x, y+1) */
    adiac[3].x = x;
    adiac[3].y = (y == pw->plan->ncol - 1 ? 0 : y + 1);

    for (i = 0; i < 4; ++i)
    {
        ad_val[i] = pw->plan->w[adiac[i].x][adiac[i].y];

        switch (ad_val[i])
        {
            case WATER:
                ++water_c;
                break;

            case SHARK:
                ++shark_c;
                break;

            case FISH:
                ++fish_c;
                break;

            default:
                errno_set(EIO, -1);
        }
    }

    /* Il pesce non trova spazi liberi, rimane fermo */
    if (water_c == 0)
    {
        *k = x;
        *l = y;

        return STOP;
    }

    while (water_c > 0)
    {
        i = rand() % 4;

        *k = adiac[i].x;
        *l = adiac[i].y;

        /* C'è almeno uno spazio libero, quindi si sposta */
        if (ad_val[i] == WATER)
        {
            pw->plan->w[x][y]   = WATER;
            pw->plan->w[*k][*l] = FISH;

            pw->plan->btime[*k][*l] = pw->plan->btime[x][y];
            pw->plan->dtime[*k][*l] = pw->plan->dtime[x][y];

            pw->plan->btime[x][y] = 0;
            pw->plan->dtime[x][y] = 0;

            return MOVE;
        }
    }

    /* Errore, water_c < 0 */
    errno_set(EIO, -1);
}

int fish_rule4 (wator_t* pw, int x, int y, int *k, int* l)
{
    point_t adiac[4];
    cell_t  ad_val[4];

    unsigned char i;
    char born = 0;

    /* Errore, struttura WATOR è uguale a NULL */
    if (pw == NULL || k == NULL || l == NULL)
    {
        errno_set(EINVAL, -1);
    }

    /* Punto (x-1, y) */
    adiac[0].x = (x == 0 ? pw->plan->nrow - 1 : x - 1);
    adiac[0].y = y;

    /* Punto (x, y-1) */
    adiac[1].x = x;
    adiac[1].y = (y == 0 ? pw->plan->ncol - 1 : y - 1);

    /* Punto (x+1, y) */
    adiac[2].x = (x == pw->plan->nrow - 1 ? 0 : x + 1);
    adiac[2].y = y;

    /* Punto (x, y+1) */
    adiac[3].x = x;
    adiac[3].y = (y == pw->plan->ncol - 1 ? 0 : y + 1);

    /* Prendiamo i valori dei punti adiacenti */
    for (i = 0; i < 4; ++i)
        ad_val[i] = pw->plan->w[adiac[i].x][adiac[i].y];

    *k = x;
    *l = y;

    /* Caso di nascita nuovo pesce */
    if (pw->plan->btime[x][y] < pw->sb)
        ++(pw->plan->btime[x][y]);

    else
    {
        pw->plan->btime[x][y] = 0;

        for (i = 0; i < 4 && !born; ++i)
        {
            if (ad_val[i] == WATER)
            {
                pw->plan->w[adiac[i].x][adiac[i].y] = FISH;
                born = 1;
            }
        }
    }

    /* La funzione termina con successo */
    return 0;
}

int fish_count(planet_t *p)
{
    int i, j;
    int count = 0;

    /* Struttura planet_t *p uguale a NULL, errore */
    if (p == NULL)
    {
        errno_set(EINVAL, -1);
    }

    for (i = 0; i < p->nrow; ++i)
    {
        for (j = 0; j < p->ncol; ++j)
        {
            if (p->w[i][j] == FISH)
                ++count;
        }
    }

    return count;
}

int shark_count(planet_t *p)
{
    int i, j;
    int count = 0;

    /* Struttura planet_t *p uguale a NULL, errore */
    if (p == NULL)
    {
        errno_set(EINVAL, -1);
    }

    for (i = 0; i < p->nrow; ++i)
    {
        for (j = 0; j < p->ncol; ++j)
        {
            if (p->w[i][j] == SHARK)
                ++count;
        }
    }

    return count;
}

int update_wator(wator_t *pw)
{
    int i, j;
    int temp_x, temp_y;

    /* Errore, struttura wator_t *pw uguale a NULL 
               oppure array d'appoggio uguale a NULL
     */
    if (pw == NULL || array == NULL)
    {
        errno_set(EINVAL,  -1);
    }

    initialize_array(pw);

    for (i = 0; i < pw->plan->nrow; ++i)
    {
        for (j = 0; j < pw->plan->ncol; ++j)
        {
            switch (pw->plan->w[i][j])
            {
                /* In caso di errori, errno è settato dalle funzioni
                   di aggiornamento.
                 */
                case SHARK:
                    /* Applica le regole se array[i][j] = 0, ovvero se
                       non sono state applicate in precedenza alla casella.
                     */
                    if (array[i][j] == 0)
                    {
                        if (shark_rule1(pw, i, j, &temp_x, &temp_y) != -1)
                            array[temp_x][temp_y] = 1;
                        else return -1;

                        if (shark_rule2(pw, temp_x, temp_y, &temp_x, &temp_y) != -1)
                            array[temp_x][temp_y] = 1;
                        else return -1;
                    }

                    break;

                case FISH:
                    if (array[i][j] == 0)
                    {
                        if (fish_rule3(pw, i, j, &temp_x, &temp_y) != -1)
                            array[temp_x][temp_y] = 1;
                        else return -1;

                        if (fish_rule4(pw, temp_x, temp_y, &temp_x, &temp_y) != -1)
                            array[temp_x][temp_y] = 1;
                        else return -1;
                    }

                    break;

                default:
                    /* Va avanti col ciclo, non ci sono regole da applicare */
                    break;
            }
        }
    }

    /* Aggiorna i contatori del Wator */
    pw->nf = fish_count(pw->plan);
    pw->ns = shark_count(pw->plan);

    if (pw->nf == -1 || pw->ns == -1)
    {
        /* Errore, errno già settato */
        return -1;
    }

    /* E' andato tutto bene, yeee! */
    ++(pw->chronon);
    return 0;
}

#endif /* __WATOR__H */

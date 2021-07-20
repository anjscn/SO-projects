// PingPongOS - PingPong Operating System
// Camile N. A. - UFPR 2021
// Uso de semáforos

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

#define VAGAS 5

// fila de inteiros para casting com queue_t
typedef struct filaint_t
{
   struct filaint_t *prev, *next;
   int item;
} filaint_t;

filaint_t *buffer;
task_t p1, p2, p3, c1, c2;
semaphore_t s_buffer, s_item, s_vaga;
int item, num_itens, num_vagas;

void produtor (void * arg)
{
    filaint_t *aux;

    while (1) {
        task_sleep (1000);
        item = rand () % 100;

        // requisita o semáforo
        sem_down (&s_vaga);
        sem_down (&s_buffer);

        // insere item no buffer
        aux = NULL;
        aux = (filaint_t*)malloc(sizeof(filaint_t));
        aux->item = item;
        queue_append ((queue_t **) &buffer, (queue_t*) aux);

        num_itens++;
        num_vagas--;

        // libera o semáforo
        sem_up (&s_buffer);
        sem_up (&s_item);

        printf ("%s produziu %d\n", (char *) arg, item);
        // printf ("(itens: %d, vagas: %d)\n", num_itens, num_vagas);
   }
}

void consumidor (void * arg)
{
    filaint_t *aux;

    while (1) {
        // requisita o semáforo
        sem_down (&s_item);
        sem_down (&s_buffer);

        // retira item do buffer
        aux = buffer;
        queue_remove ((queue_t **) &buffer, (queue_t*) aux);

        num_itens--;
        num_vagas++;
        
        // libera o semáforo
        sem_up (&s_buffer);
        sem_up (&s_vaga);

        printf ("%s consumiu %d\n", (char *) arg, aux->item);
        // printf ("                           (itens: %d, vagas: %d)\n", num_itens, num_vagas);

        task_sleep (1000);
    }
}

int main (int argc, char *argv[])
{
    num_itens = 0;
    num_vagas = VAGAS;
    buffer = NULL;

    ppos_init ();

    // cria semáforos
    sem_create (&s_buffer, 1);
    sem_create (&s_item, 0);
    sem_create (&s_vaga, VAGAS);

    // cria produtores
    task_create (&p1, produtor, "p1");
    task_create (&p2, produtor, "p2");
    task_create (&p3, produtor, "p3");

    // cria consumidores
    task_create (&c1, consumidor, "                           c1");
    task_create (&c2, consumidor, "                           c2");

    task_join (&p1);
    // task_join (&p2);
    // task_join (&p3);
    // task_join (&c1);
    // task_join (&c2);

    // destroi semaforos
    // sem_destroy (&s_buffer);
    // sem_destroy (&s_item);
    // sem_destroy (&s_vaga);

    task_exit (0);

    exit (0);
}

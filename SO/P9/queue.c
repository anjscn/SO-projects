// Implementação de fila genérica
// Camile N. A.

#include <stdio.h>
#include "queue.h"

int queue_size (queue_t *queue)
{
    int size = 0;
    
    if (queue) { // Existe uma fila
        size++;
        if (&queue->prev != &queue) { // Existe mais de um elemento na fila
            for (queue_t *aux = queue->next; aux != queue; aux = aux->next) {
                size++;
            }
        }
    }
    return size;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*) )
{
    queue_t *aux;

    fprintf (stdout, "%s: [", name);
    if (queue) {
        for (aux = queue; aux != queue->prev; aux = aux->next) {
            print_elem ((void*) aux);
            fprintf (stdout, " ");
        }
        print_elem ((void*) aux); // Ultimo elemento
    }
    fprintf (stdout, "] \n");
}

int queue_append (queue_t **queue, queue_t *elem)
{
    if (queue && elem) { // Fila e elemento existem
        if (!(elem->next && elem->prev)) { // Elemento esta isolado
            if (!*queue) { // Fila vazia
                (*queue) = elem;
                (*queue)->prev = elem;
                (*queue)->next = elem;
            } else {
                (*queue)->prev->next = elem; // Elem entra no fim da fila
                elem->prev = (*queue)->prev;
                (*queue)->prev = elem; // O prev da fila aponta para o elemento novo
                elem->next = (*queue);
            }
            return 0;
        }
        fprintf(stderr, "Erro! O elemento nao esta isolado \n");
        return -1;
    }
    fprintf(stderr, "Erro! A fila ou o elemento nao existem \n");
    return -1;
}

int queue_remove (queue_t **queue, queue_t *elem)
{
    if (queue && (*queue)->prev) { // Fila existe e nao esta vazia
        if (elem) { // Elemento existe
            queue_t *aux;

            if ((*queue) == elem) { // Elemento eh o primeiro da fila
                if (elem->prev == elem) { // Um elemento na fila
                    elem->prev = NULL;
                    elem->next = NULL;
                    (*queue) = NULL;
                } else {
                    aux = (*queue)->next;
                    elem->next->prev = (*queue)->prev;
                    elem->prev->next = (*queue)->next;
                    elem->prev = NULL;
                    elem->next = NULL;
                    (*queue) = aux;
                }
                return 0;
            } else {
                for (aux = (*queue)->next; aux != (*queue)->next->prev; aux = aux->next) { // Procura elemento na fila
                    if (aux == elem) {
                        elem->next->prev = aux->prev;
                        elem->prev->next = aux->next;
                        elem->prev = NULL;
                        elem->next = NULL;
                        return 0;
                    }
                }
            }
            fprintf(stderr, "Erro! O elemento nao pertence a fila \n");
            return -1;
        }
        fprintf(stderr, "Erro! O elemento nao existe \n");
        return -1;
    }
    fprintf(stderr, "Erro! A fila esta vazia ou nao existe \n");
    return -1;
}

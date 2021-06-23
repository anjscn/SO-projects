// PingPongOS - PingPong Operating System
// Camile N. A. - UFPR 2021
// Escalonador por prioridades

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

task_t *scheduler ();
void task_dispatcher ();

task_t Main, dispatcher, *fila, *current;
int id;

void ppos_init ()
{
    getcontext (&Main.context);
    current = (task_t*)malloc(sizeof(task_t));

    fila = NULL;

    id = 0;
    task_create (&dispatcher, (void*) task_dispatcher, "Dispatcher");
    setvbuf (stdout, 0, _IONBF, 0);
}

int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
    if (task) {
        getcontext (&task->context); // Salva o contexto atual na tarefa

        char *stack = malloc(STACKSIZE);
        if (stack) {
            task->context.uc_stack.ss_sp = stack;
            task->context.uc_stack.ss_size = STACKSIZE;
            task->context.uc_stack.ss_flags = 0;
            task->context.uc_link = 0;
            makecontext (&task->context, (void*)(*start_func), 1, (char *)arg);

            if (id > 0) {
                if (!queue_append ((queue_t **) &fila, (queue_t*) task)); // Adiciona a tarefa no fim da fila
                else 
                    return -1;
            }
            id++;
            task->id = id;
            task->status = 1;
            task->p_est = 0;
            task->p_din = task->p_est;
            return(task->id);
        }
    }
    return -1;
}

void task_yield ()
{
    task_switch (&dispatcher);
}

void task_dispatcher ()
{
    task_t *task;

    while (fila) {
        task = scheduler ();
        if (task) {
            task_switch (task);
        }
    }
    task_exit(0);
}

task_t *scheduler ()
{
    task_t *aux, *task;
    aux = fila;
    task = fila;

    int i, user_tasks = MAX;

    while (user_tasks > 0) {
        if (aux->p_din < aux->next->p_din) {
            if (task) {
                if (aux->p_din < task->p_din)
                    task = aux;
            }
            else
                task = aux;
        }
        user_tasks--;
        aux = aux->next;
    }

    for (i = 0; i < MAX; i++) {
        aux->p_din--;
        if (aux->p_din <= -20) {
            task->p_din = task->p_est;
        }
        aux = aux->next;
    }

    task->p_din = task->p_est;
    
    return (task);
}


int task_switch (task_t *task)
{
    task_t *aux0;

    aux0 = current;
    current = task;

    if (swapcontext (&aux0->context, &task->context) < 0) // Salva contexto da tarefa e troca para o atual
        return -1;
    if (fila)
        fila = fila->next;
    return 0;
}

void task_exit (int exit_code)
{
    if (current == &dispatcher) {
        task_switch (&Main);
    }
    else {
        current->status--;
        queue_remove ((queue_t **) &fila, (queue_t*) current);
        task_switch (&dispatcher);
    }
}

int task_id ()
{
    return (current->id);
}

void task_setprio (task_t *task, int prio)
{        
    if ((prio > -20) && (prio < 20)) {
        if  (!task) 
            task = current;
        task->p_est = prio;
        task->p_din = task->p_est;
    }
    else
        task_exit(0);
}

int task_getprio (task_t *task)
{
    if (!task)
        task = current;
    return (task->p_est);
}

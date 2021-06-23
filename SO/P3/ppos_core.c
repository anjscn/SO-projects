// PingPongOS - PingPong Operating System
// Camile N. A. - UFPR 2021
// Dispatcher

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

task_t * scheduler ();
void task_dispatcher ();

task_t Main, *fila, *current, dispatcher;
int id;

void ppos_init ()
{
    getcontext (&Main.context);

    current = (task_t*)malloc(sizeof(task_t));
    fila = NULL;
    Main.id = 0;
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
            makecontext (&task->context, (void*)(*start_func), 1, arg);

            if (id > 0) {
                if (!queue_append ((queue_t **) &fila, (queue_t*) task)); // Adiciona a tarefa no fim da fila
                else 
                    return -1;
            }
            id++;
            task->id = id;
            task->state = 0;
            return(task->id);
        }
    }
    return -1;
}

void task_yield ()
{
    task_switch(&dispatcher);
    fila = fila->next;
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


task_t * scheduler ()
{   
    return (fila);
}


int task_switch (task_t *task)
{
    task_t *aux0;

    aux0 = current;
    current = task;

    if (swapcontext (&aux0->context, &task->context) < 0) 
        return -1;
    return 0;
}

void task_exit (int exit_code)
{
    if (current == &dispatcher) { 
        task_switch (&Main);
    }
    else {
        current->state--; 
        queue_remove ((queue_t **) &fila, (queue_t*) current);
        task_switch (&dispatcher);
    }
}

int task_id ()
{
    return (current->id);
}

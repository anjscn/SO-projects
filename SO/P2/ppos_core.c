// PingPongOS - PingPong Operating System
// Camile N. A. - UFPR 2021
// Gestão de tarefas

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

task_t Main, *current;
int id;

void ppos_init ()
{
    getcontext (&Main.context);
    current = (task_t*)malloc(sizeof(task_t));
    
    Main.id = 0;
    id = 0;
    setvbuf (stdout, 0, _IONBF, 0);
}

int task_create (task_t *task, void (*start_func)(void *), void *arg)
{
    if (task) {
        getcontext (&task->context);

        char *stack = malloc(STACKSIZE);
        if (stack) {
            task->context.uc_stack.ss_sp = stack;
            task->context.uc_stack.ss_size = STACKSIZE;
            task->context.uc_stack.ss_flags = 0;
            task->context.uc_link = 0;

            if (start_func) {
                makecontext (&task->context, (void*)(*start_func), 1, arg);
                id++;
                task->id = id;
                #ifdef DEBUG
                    printf ("task_create: criou tarefa %d\n", task->id) ;
                #endif
                return(task->id);
            }
        }
    }
    return(-1);
}

int task_switch (task_t *task)
{
    #ifdef DEBUG
        printf ("task_switch: alternou para a tarefa %d\n", task->id) ;
    #endif
    task_t *aux0;

    aux0 = current;
    current = task;

    if (swapcontext (&aux0->context, &task->context) < 0) // Salva contexto da tarefa e troca para o atual
        return -1;
    return 0;
}

void task_exit (int exit_code)
{
    setcontext(&Main.context);
}

int task_id ()
{
    return (current->id);
}

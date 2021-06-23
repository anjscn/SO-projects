// PingPongOS - PingPong Operating System
// Camile N. A. - UFPR 2021
// Contabilização

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

task_t *scheduler ();
void task_dispatcher ();
unsigned int systime ();

task_t Main, dispatcher, *fila, *current;
unsigned int id, tempo, strproc;

unsigned int systime ()
{
    return (tempo);
}

// Tratador do sinal
void tratador (int signum)
{   
    tempo++;
    if (current->flag) {
        if (current->quantum > 0) 
            current->quantum--;
        else 
            task_yield ();
    }
}

void ppos_init ()
{
    getcontext (&Main.context);
    current = (task_t*)malloc(sizeof(task_t));
    fila = NULL;
    Main.id = 0;
    id = 0;
    tempo = 0;
    task_create (&dispatcher, (void*) task_dispatcher, "Dispatcher");
    setvbuf (stdout, 0, _IONBF, 0);

    // Registra a ação para o sinal de timer SIGALRM
    action.sa_handler = tratador;
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction (SIGALRM, &action, 0) < 0) {
        perror ("Erro em sigaction: ");
        exit (1);
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000;      // Primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0;      // Primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000;   // Disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0;   // Disparos subsequentes, em segundos

    // Arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0) {
        perror ("Erro em setitimer: ");
        exit (1);
    }
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

            if (id > 0) { // Dispacher não entra na fila
                if (!queue_append ((queue_t **) &fila, (queue_t*) task)); // Adiciona a tarefa no fim da fila
                else 
                    return -1;
                task->status = 1;
                task->p_est = -30;
                task->p_din = task->p_est;
                task->flag = 1;
                task->quantum = 0;
            }
            else { 
                task->flag = 0; // Sinaliza tarefa de sistema
            }
            id++;
            task->id = id;
            task->strtime = systime(); // Tempo de inicio da tarefa
            task->acumulador = 0; // Acumulador
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
        if (current->p_est > -20) { // Com prioridade
            task = scheduler ();
            if (task) {
                task->quantum = 20;
                task_switch (task);
            }
        }
        else { // Sem prioridade
            task = fila;
            task->quantum = 20;
            task_switch (task);
        }
    }
    task_exit(0);
}

task_t *scheduler () // Escalonador
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

    for (i = 0; i < MAX; i++) { // Envelhecimento
        aux->p_din--;
        if (aux->p_din <= -20) {
            task->p_din = task->p_est;
        }
        aux = aux->next;
    }

    task->p_din = task->p_est;
    
    return (task);
}

int task_switch (task_t *task) // Troca o contexto da tarefa atual
{
    task_t *aux0;

    current->acumulador = current->acumulador + (systime() - strproc);

    aux0 = current;
    current = task;

    if (swapcontext (&aux0->context, &task->context) < 0) 
        return -1;
    strproc = systime();
    current->ativacoes++;
    if (fila)
        fila = fila->next;
    return 0;
}

void task_exit (int exit_code)
{
    if (current == &dispatcher) { // Nenhuma tarefa na fila
        printf ("Task %d exit: execution time %d ms, processor time  %d ms, %d activations\n", 
            current->id,
            systime() - current->strtime, 
            current->acumulador, 
            current->ativacoes
        );
        task_switch (&Main);
    }
    else { // Uma tarefa acabou a execução
        current->status--;
        printf ("Task %d exit: execution time %d ms, processor time  %d ms, %d activations\n", 
            current->id,
            systime() - current->strtime, 
            current->acumulador, 
            current->ativacoes
        );
        queue_remove ((queue_t **) &fila, (queue_t*) current);
        task_switch (&dispatcher);
    }
}

int task_id ()
{
    return (current->id);
}

void task_setprio (task_t *task, int prio) // Define a prioridade da tarefa
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

int task_getprio (task_t *task) // Retorna a prioridade da tarefa
{
    if (!task)
        task = current;
    return (task->p_est);
}

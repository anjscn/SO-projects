// PingPongOS - PingPong Operating System
// Camile N. A. - UFPR 2021
// Operador Join

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

task_t *scheduler ();
void task_dispatcher ();
unsigned int systime ();

task_t Main, dispatcher, *ready_queue, *suspended_queue, *current;
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
        if (current->quantum > 1)
            current->quantum--;
        else
            task_yield ();
    }
}

void ppos_init ()
{
    current = (task_t*)malloc(sizeof(task_t));
    ready_queue = NULL;
    suspended_queue = NULL;
    tempo = 0;

    // Tarefa Main
    queue_append ((queue_t **) &ready_queue, (queue_t*) &Main);
    getcontext (&Main.context);
    Main.status = 1;
    Main.p_est = -30;
    Main.p_din = Main.p_est;
    Main.flag = 1;
    Main.quantum = 0;
    Main.id = 0;
    Main.strtime = systime();
    Main.acumulador = 0;

    id = 0;
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

    // Ajusta valores do temporizador
    timer.it_value.tv_usec = 1000;      // Primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0;      // Primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000;   // Disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0;   // Disparos subsequentes, em segundos

    // Arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0) {
        perror ("Erro em setitimer: ");
        exit (1);
    }

    // Ativa o dispatcher
    task_yield ();
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

            if (id > 0) { // Dispacher não entra na fila de tarefas prontas
                if (!queue_append ((queue_t **) &ready_queue, (queue_t*) task)); // Adiciona a tarefa no fim da fila
                else 
                    return -1;
                task->status = 1;
                task->flag = 1;
            }
            else { 
                task->status = 0;
                task->flag = 0; // Sinaliza tarefa de sistema
            }
            id++;
            task->id = id;
            task->p_est = -30;
            task->p_din = task->p_est;
            task->quantum = 0;
            task->strtime = systime();
            task->acumulador = 0;
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

    while (ready_queue) {
        if (current->p_est > -20) { // Com prioridade
            task = scheduler ();
            if (task)
                task_switch (task);
        }
        else { // Sem prioridade
            task = ready_queue;
            if (task){
                task_switch (task);
                if (ready_queue)
                    ready_queue = ready_queue->next;
            }
        }
    }
    task_exit(0);
}

task_t *scheduler () // Escalonador
{
    task_t *aux, *task;
    aux = ready_queue;
    task = ready_queue;

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
    current->quantum = 20;

    if (swapcontext (&aux0->context, &task->context) < 0) 
        return -1;

    strproc = systime();
    current->ativacoes++;

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
        current->status = 0;
        printf ("Task %d exit: execution time %d ms, processor time  %d ms, %d activations\n",
            current->id,
            systime() - current->strtime,
            current->acumulador,
            current->ativacoes
        );
        queue_remove ((queue_t **) &ready_queue, (queue_t*) current);
        // Tarefas suspensas voltam para a fila de prontas
        if (suspended_queue) {
            task_t *aux = suspended_queue;
            while (aux) {
                queue_remove ((queue_t **) &suspended_queue, (queue_t*) aux);
                queue_append ((queue_t **) &ready_queue, (queue_t*) aux);
                aux->status = 1;
                aux = suspended_queue;
            }
        }
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

int task_join (task_t *task)
{
    if (task && (task->status != 0)) {
        current->status = -1;
        queue_remove ((queue_t **) &ready_queue, (queue_t*) current);
        queue_append ((queue_t **) &suspended_queue, (queue_t*) current);
        task_yield ();
        return task->id;
    }
    return -1;
}

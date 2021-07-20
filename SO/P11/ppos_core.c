// PingPongOS - PingPong Operating System
// Camile N. A. - UFPR 2021
// Uso de semáforos

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

task_t *scheduler ();
void task_dispatcher ();
void task_wakeup ();
task_t *next_task ();
unsigned int systime ();

task_t Main, dispatcher, *ready_queue, *suspended_queue, *sleeping_queue, *current;
unsigned int id, clock, start_proc, user_tasks, sleeping_tasks, lock;

unsigned int systime ()
{
    return (clock);
}

// Tratador do sinal
void tratador (int signum)
{   
    clock++;
    if ((current != &dispatcher) && !lock) {
        if (current->quantum > 1)
            current->quantum--;
        else
            task_yield ();
    }
}

void ppos_init ()
{
    setvbuf (stdout, 0, _IONBF, 0);
    current = (task_t*)malloc(sizeof(task_t));
    ready_queue = NULL;
    suspended_queue = NULL;
    sleeping_queue = NULL;
    clock = 0;
    lock = 0;

    // Tarefa Main
    queue_append ((queue_t **) &ready_queue, (queue_t*) &Main);
    getcontext (&Main.context);
    Main.status = 1;
    Main.prio_est = 0;
    Main.prio_din = Main.prio_est;
    Main.quantum = 0;
    Main.id = 0;
    Main.start_task = systime();
    Main.acumulador = 0;

    user_tasks = 1;
    sleeping_tasks = 0;
    id = 0;

    // Tarefa Dispatcher
    task_create (&dispatcher, (void*) task_dispatcher, "Dispatcher");
    dispatcher.status = 0;

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

/*
Identificador status:
    0- Tarefa terminou execução ou não executa;
    1- Tarefa pronta para executar;
    2- Tarefa suspensa;
    3- Tarefa adormecida.
*/

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
            // Tarefa de usuário
            if (task != &dispatcher) {
                queue_append ((queue_t **) &ready_queue, (queue_t*) task);
                task->status = 1;
                user_tasks++;
            }
            id++;
            task->id = id;
            task->prio_est = 0;
            task->prio_din = task->prio_est;
            task->quantum = 0;
            task->start_task = systime();
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

    while (user_tasks > 0) {
        task = scheduler ();
        if (task) {
            task->quantum = 20;
            task_switch (task);
            switch (task->status) {
                case 1: ready_queue = ready_queue->next; break;
            }
        }
        task_wakeup ();
    }
    task_exit(0);
}

task_t *scheduler () // Escalonador
{
    task_t *aux, *task;
    aux = ready_queue;
    task = ready_queue;

    if (ready_queue) {
        for (int i = 0; i < user_tasks; i++) {
            if (aux->prio_din < aux->next->prio_din) {
                if (task) {
                    if (aux->prio_din < task->prio_din)
                        task = aux;
                }
                else
                    task = aux;
            }
            aux = aux->next;
        }
        aux = ready_queue;
        for (int i = 0; i < user_tasks; i++) { // Envelhecimento
            aux->prio_din--;
            if (aux->prio_din <= -20) {
                task->prio_din = task->prio_est;
            }
            aux = aux->next;
        }
        task->prio_din = task->prio_est;
    }
    return (task);
}

int task_switch (task_t *task) // Troca o contexto da tarefa atual
{
    task_t *aux0;

    current->acumulador = current->acumulador + (systime() - start_proc);
    aux0 = current;
    current = task;

    if (swapcontext (&aux0->context, &task->context) < 0)
        return -1;

    start_proc = systime();
    current->ativacoes++;

    return 0;
}

void task_exit (int exit_code)
{
    // Nenhuma tarefa na fila
    if (current == &dispatcher) {
        printf ("Task %d exit: execution time %d ms, processor time  %d ms, %d activations\n", current->id, systime() - current->start_task, current->acumulador, current->ativacoes);
        task_switch (&Main);
    }
    // Uma tarefa acabou a execução
    else {
        // Tarefas suspensas voltam para a fila de prontas
        if (suspended_queue) {
            task_t *aux;
            aux = suspended_queue;
            while (aux) {
                queue_remove ((queue_t **) &suspended_queue, (queue_t*) aux);
                queue_append ((queue_t **) &ready_queue, (queue_t*) aux);
                aux->status = 1;
                aux = suspended_queue;
            }
        }
        queue_remove ((queue_t **) &ready_queue, (queue_t*) current);
        user_tasks--;
        current->status = 0;
        printf ("Task %d exit: execution time %d ms, processor time  %d ms, %d activations\n", current->id, systime() - current->start_task, current->acumulador, current->ativacoes);
        task_yield ();
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
        task->prio_est = prio;
        task->prio_din = task->prio_est;
    }
    else
        task_exit(0);
}

int task_getprio (task_t *task) // Retorna a prioridade da tarefa
{
    if (!task)
        task = current;
    return (task->prio_est);
}

int task_join (task_t *task) // Suspende a execução da tarefa atual até a conclusão da *task
{
    if (task && (task->status == 1)) {
        queue_remove ((queue_t **) &ready_queue, (queue_t*) current);
        queue_append ((queue_t **) &suspended_queue, (queue_t*) current);
        current->status = 2;
        task_yield ();
        return task->id;
    }
    return -1;
}

void task_sleep (int t) // Coloca as tarefas pra dormir
{
    if ((t > 0) && (current->status == 1)) {
        queue_remove ((queue_t **) &ready_queue, (queue_t*) current);
        queue_append ((queue_t **) &sleeping_queue, (queue_t*) current);
        current->status = 3;
        current->wakeup = systime() + t;
        sleeping_tasks++;
        task_yield ();
    }
}

void task_wakeup () // Acorda as tarefas
{
    if (sleeping_queue) {
        task_t *aux1, *aux2;

        aux1 = sleeping_queue;
        for (int i = 0; i < sleeping_tasks; i++) {
            aux2 = sleeping_queue->next;
            if (aux1->wakeup <= systime()) {
                queue_remove ((queue_t **) &sleeping_queue, (queue_t*) aux1);
                queue_append ((queue_t **) &ready_queue, (queue_t*) aux1);
                aux1->status = 1;
                sleeping_tasks--;
                aux1 = aux2;
            }
        }
    }
}

int sem_create (semaphore_t *s, int value) // Cria semáforo
{
    if (s) {
        lock = 1; // Operação atômica
        s->counter = value;
        s->semaphore_queue = NULL;
        s->status = 1;
        lock = 0; // Operação atômica
        return 0;
    }
    return -1;
}

int sem_down (semaphore_t *s) // Suspende semáforo
{
    if (s && (s->status == 1)) {
        lock = 1; // Operação atômica
        s->counter--;
        if (s->counter < 0) {
            queue_remove ((queue_t **) &ready_queue, (queue_t*) current);
            queue_append ((queue_t **) &s->semaphore_queue, (queue_t*) current);
            current->status = 2;
            lock = 0; // Operação atômica
            task_yield ();
        }
        return 0;
    }
    return -1;
}

int sem_up (semaphore_t *s) // Semáforo volta a executar
{
    if (s && (s->status == 1)) {
        lock = 1; // Operação atômica
        task_t *aux;
        s->counter++;
        if (s->semaphore_queue && s->counter <= 0) {
            aux = s->semaphore_queue;
            queue_remove ((queue_t **) &s->semaphore_queue, (queue_t*) aux);
            queue_append ((queue_t **) &ready_queue, (queue_t*) aux);
            aux->status = 1;
        }
        lock = 0; // Operação atômica
        return 0;
    }
    return -1;
}

int sem_destroy (semaphore_t *s) // Destrói semáforo
{
    if (s && (s->status == 1)) {
        lock = 1; // Operação atômica
        task_t *aux;
        while (s->semaphore_queue != NULL) {
            aux = s->semaphore_queue;
            queue_remove ((queue_t **) &s->semaphore_queue, (queue_t*) aux);
            queue_append ((queue_t **) &ready_queue, (queue_t*) aux);
        }
        s->status = 0;
        lock = 0; // Operação atômica
        return 0;
    }
    return -1;
}

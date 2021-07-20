// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__
#define STACKSIZE 64*1024
#define DEBUG

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas
#include <signal.h>
#include <sys/time.h>

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next ;		// ponteiros para usar em filas
  int id, status, prio_est, prio_din, quantum, start_task, acumulador, ativacoes, wakeup ;				// identificador da tarefa
  ucontext_t context ;			// contexto armazenado da tarefa
} task_t ;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

// estrutura de inicialização to timer
struct itimerval timer ;

// estrutura que define um semáforo
typedef struct
{
  task_t *semaphore_queue ;
  int counter, status ;
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif

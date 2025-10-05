//
// Created by Bruno on 04/10/2025.
// 3 níveis de prioridade: 0 (alta), 1 (média), 2 (baixa)
// Cada nível funciona como Round Robin com um time-slice de 500 ms
//

#include "mlfq.h"
#include <stdio.h>
#include <stdlib.h>
#include "msg.h"
#include <unistd.h>

#define NUM_LEVELS 3          // 3 filas de prioridade
#define TIME_SLICE 500        // cada processo tem 500 ms de CPU antes de ser trocado

// Criação das filas de prioridade (iniciadas a NULL)
static queue_t level_queues[NUM_LEVELS] = {
    {NULL, NULL}, {NULL, NULL}, {NULL, NULL}
};

void mlfq_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {

    // Passo 1: mover processos novos da ready_queue para a fila de prioridade 0
    while (rq->head != NULL) {
        pcb_t *task = dequeue_pcb(rq);
        task->priority = 0;
        enqueue_pcb(&level_queues[0], task);
    }
    if (*cpu_task != NULL) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS; // soma o tempo passado desde o último tick

        // Verifica há quanto tempo está no slice atual
        uint32_t used_slice = current_time_ms - (*cpu_task)->slice_start_ms;

        // Caso 1: processo terminou o burst (acabou o tempo total)
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            msg_t msg = {
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };

            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }

            // liberta o processo porque já terminou
            free(*cpu_task);
            *cpu_task = NULL;
        }

        // Caso 2: se o time-slice expirou (usou os 500 ms)
        else if (used_slice >= TIME_SLICE) {
            // desce de prioridade (máx: nível 2)
            if ((*cpu_task)->priority < NUM_LEVELS - 1) {
                (*cpu_task)->priority++;
            }

            // volta para a sua fila correspondente (no fim → efeito RR)
            enqueue_pcb(&level_queues[(*cpu_task)->priority], *cpu_task);

            // liberta o CPU para dar oportunidade a outros
            *cpu_task = NULL;
        }

        // Caso 3: se ainda não acabou nem o slice terminou → continua no CPU
    }

    // Passo 3: se o CPU está livre, escolhe o próximo processo
    if (*cpu_task == NULL) {
        for (int level = 0; level < NUM_LEVELS; level++) {
            if (level_queues[level].head != NULL) {
                *cpu_task = dequeue_pcb(&level_queues[level]);
                (*cpu_task)->slice_start_ms = current_time_ms; // reinicia o contador do slice
                (*cpu_task)->status = TASK_RUNNING;            // marca como em execução
                break;
            }
        }
    }
}

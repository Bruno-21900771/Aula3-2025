//
// Created by bruno on 04/10/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include "rr.h"
#include "msg.h"
#include <unistd.h>

#define TIME_SLICE_MS 500

void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;      // Add to the running time of the application/task
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            // Task finished
            // Send msg to application
            msg_t msg = {
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            // Application finished and can be removed (this is FIFO after all)
            free((*cpu_task));
            (*cpu_task) = NULL;
        }
        // Verifica se o time-slice do processo expirou
        // (já correu os 500ms que tinha direito)
        else if (current_time_ms - (*cpu_task)->slice_start_ms >= TIME_SLICE_MS) {
            // Coloca o processo no final da fila (round-robin)
            enqueue_pcb(rq, *cpu_task);
            (*cpu_task) = NULL;  // Liberta o CPU
        }
    }
    // Se o CPU está livre e há processos à espera
    if (*cpu_task == NULL && rq->head != NULL) {
        // Retira o primeiro processo da fila (FIFO)
        *cpu_task = dequeue_pcb(rq);

        // Atualiza o estado do processo
        (*cpu_task)->status = TASK_RUNNING;
        (*cpu_task)->slice_start_ms = current_time_ms;  // Marca quando começou este slice
    }
}
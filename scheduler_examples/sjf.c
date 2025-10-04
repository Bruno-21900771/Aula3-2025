//
// Created by bruno on 04/10/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include "sjf.h"
#include "msg.h"
#include <unistd.h>

void sjf_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
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
    }
    if (*cpu_task == NULL && rq->head != NULL) {

        queue_elem_t *it = rq->head, *shortest = rq ->head;
        while (it) {
            if (it -> pcb ->time_ms < shortest -> pcb ->time_ms) {
                it = it -> next;
            }
        }
        queue_elem_t *removed = remove_queue_elem(rq, shortest);
        *cpu_task = removed ->pcb;
        free(removed);

        (*cpu_task) -> status = TASK_RUNNING;
        (*cpu_task) -> slice_start_ms = current_time_ms;

    }
}

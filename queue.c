#include <stdint.h>
#include <semaphore.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include "queue.h"

#define BUFF_LEN 128
#define MAX_PRIORITY 10


typedef struct queue_sem {
	sem_t mutex;			//
	sem_t empty;			// blocks on empty starts at 0
	sem_t full[MAX_PRIORITY] ;	// blocks on full in bucket starts at BUFF_LEN
} queue_sem;

typedef struct queue {
	enum {pusta, cos, pelna} status;
	char* q[BUFF_LEN];
	uint16_t q_start,
		 q_end;
} queue_type ; /*[MAX_PRIORITY] = { {
	.status = pusta,
} }; */

queue_type *queue;
queue_sem *sems;
/*
static uint16_t	q_start[MAX_PRIORITY] = {0},
       		q_end[MAX_PRIORITY] = {0};
		*/

void QInit(void) {
	if((queue = (queue_type*) mmap(0, MAX_PRIORITY* sizeof(*queue), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) < 0)
		perror("mmaping queues"), exit(-1);
	if((sems = (queue_sem*) mmap(0, sizeof(queue_sem), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) < 0)
		perror("mmaping sems"), exit(-1);

	if(sem_init(&sems->mutex, 1, 1) < 0) // one process in critical section
		perror("mutex initialisation"), exit(-1);
	if(sem_init(&sems->empty, 1, 0) < 0) // queue is initially empty
		perror("empty initialisation"), exit(-1);
	for(unsigned i = 0; i < MAX_PRIORITY; i++) {
		if(sem_init(&sems->full[i], 1, BUFF_LEN) < 0) // all queues have BUFF_LEN free spaces
			perror("full initialisation"), exit(-1);

		//queue[i] = { .status = pusta };
		queue[i].status = pusta;
		queue[i].q_start = queue[i].q_end = 0;
	}
}

int QSend(unsigned priority, char* enq) {
	sem_wait(&sems->full[priority]);
	sem_wait(&sems->mutex);

	int status_ok = ! (queue[priority].status == pelna);
	if(status_ok) {
		queue[priority].q[queue[priority].q_end++ & BUFF_LEN-1] = enq;
		queue[priority].status = (queue[priority].q_end & BUFF_LEN-1) == (queue[priority].q_start & BUFF_LEN-1) ? pelna : cos;
	}
	printf("sent %u\n", priority);

	sem_post(&sems->mutex);
	sem_post(&sems->empty);
	return status_ok;
}

static char* QRecFrom(unsigned priority) { // non thread safe!
	//printf("getting from %u\n", priority);
	if(queue[priority].status == pusta)
		return 0; //err
	char *ret = queue[priority].q[queue[priority].q_start++ & BUFF_LEN-1];
	queue[priority].status = (queue[priority].q_start & BUFF_LEN-1) == (queue[priority].q_end & BUFF_LEN-1) ? pusta : cos;
	return ret;
}

char* QRec(void) {
	char* ret = 0;
	unsigned i;
	sem_wait(&sems->empty);
	sem_wait(&sems->mutex);
	for(i = 0; i <= MAX_PRIORITY; i++) {
		//printf("checking %d\n", MAX_PRIORITY-i);
		if(queue[MAX_PRIORITY-i].status != pusta) {
			ret = QRecFrom(MAX_PRIORITY-i);
			break;
		}
	}
	printf("rec  %u\n", MAX_PRIORITY-i);
	sem_post(&sems->mutex);
	sem_post(&sems->full[MAX_PRIORITY-i]);

	return ret;
}

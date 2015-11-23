#include <stdint.h>
#include <semaphore.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include "queue.h"


typedef struct queue_sem {
	sem_t mutex;			//
	sem_t empty;			// blocks on empty starts at 0
	sem_t full[MAX_PRIORITY] ;	// blocks on full in bucket starts at BUFF_LEN
} queue_sem;

typedef struct queue {
	enum {pusta, cos, pelna} status;
	int q[BUFF_LEN];
	uint16_t q_start,
		 q_end;
} queue_type;

queue_type *queue;
queue_sem *sems;

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

		queue[i].status = pusta;
		queue[i].q_start = queue[i].q_end = 0;
	}
}


void QPrintQueue(void) {
	sem_wait(&sems->mutex);
	printf("Queue ===\n");
	for(int i = 0; i <= MAX_PRIORITY; i++) {
		unsigned size = 0;
		printf("pri : %d\n", i);
		if(queue[i].status != pusta)
			for(uint16_t it = queue[i].q_start; QPos(it) != QPos(queue[i].q_end) || (queue[i].status == pelna && size != BUFF_LEN); it = QPos(it+1)) {
				int shm_key = queue[i].q[QPos(it)];
				char *ptr;
				if((ptr = (char*) shmat(shm_key, 0, SHM_RDONLY)) == (char*) -1)
					perror("shmat in QPrintQueue"), exit(-1);
				printf("\t%u: \"%s\" [%d]\n", ++size, ptr, queue[i].q[QPos(it)]);
				shmdt(ptr);
			}
		printf("\t\tsize: %u\n", size);
	}
	printf("=========\n");
	fflush(stdout);
	sem_post(&sems->mutex);
}


int QSend(unsigned priority, int enq) {
	sem_wait(&sems->full[priority]);
	sem_wait(&sems->mutex);
	printf("inserting ");
	int status_ok = ! (queue[priority].status == pelna);
	if(status_ok) {
		queue[priority].q[QPos(queue[priority].q_end++)] = enq;
		queue[priority].status = QPos(queue[priority].q_end) == QPos(queue[priority].q_start) ? pelna : cos;
	}
	printf("qsent %u [%d]\n", priority, queue[priority].q[QPos(queue[priority].q_end-1)]);

	sem_post(&sems->mutex);
	sem_post(&sems->empty);
	return status_ok;
}

static int QRecFrom(unsigned priority) { // non thread safe!
	if(queue[priority].status == pusta)
		return -1; //err
	int ret = queue[priority].q[QPos(queue[priority].q_start)];
	queue[priority].q[QPos(queue[priority].q_start++)] = -2; // clear mostly or debug puproses
	queue[priority].status = QPos(queue[priority].q_start) == QPos(queue[priority].q_end) ? pusta : cos;
	return ret;
}

int QRec(void) {
	int ret = -1;
	unsigned i;
	sem_wait(&sems->empty);
	sem_wait(&sems->mutex);
	for(i = 0; i <= MAX_PRIORITY; i++) {
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

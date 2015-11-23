#include "queue.h"
#include <stdio.h>
#include <unistd.h>

typedef void(* consumer_function)(unsigned);

#define CONSUMER_NUMBER 100


int main(int argc, char** argv) {
	char* std;
	QInit();

	// producer from now on
	printf("I am the producer now.\n");
	
	unsigned it = 9;
	while(1) {
		usleep(20003);

		char *mesg;
		int shm_key = shmget(IPC_PRIVATE, 100*sizeof(*mesg), IPC_CREAT | 0600);
		mesg = shmat(shm_key, 0, 0);
		sprintf(mesg, "foo %u", it);
		mesg = 0;

		int sent_ok = QSend(it++% MAX_PRIORITY, shm_key);
		printf("sent %s\n", sent_ok ? "ok" : "failed");
		QPrintQueue();
		shmdt(mesg);
	}
}

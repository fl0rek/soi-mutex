#include "queue.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

typedef void(* consumer_function)(unsigned);

#define CONSUMER_NUMBER 100
consumer_function consumer[CONSUMER_NUMBER] = { 0 };

void basic_read(unsigned myid) {
	printf("consumer %u\n", myid);
	while(1) {
		char* msg;

		usleep(100000);
		printf("PRE  ");
		QPrintQueue();
		int shm_key = QRec();
		
		if((msg = shmat(shm_key, 0, SHM_RDONLY)) == (char*) -1)
			perror("consumer shmat failed"), exit(-1);
		printf("received %u: %s\n", myid, msg);
		printf("POST ");
		QPrintQueue();

		msg = 0;
		shmdt(msg);
		if(shmctl(shm_key, IPC_RMID, (struct shmid_ds*) 0) == -1)
			perror("error releasing"), exit(-1);
	}
}


int main(int argc, char** argv) {
	char* std;
	QInit();

	consumer[0] = 
		consumer[1] = 
		consumer[2] = 
		consumer[3] = 
			basic_read;

	unsigned it = 0;
	while(consumer[it]) {
		if(fork() == 0) { // consumer number $it
			consumer[it](it);
			exit(0);
		}
		it++;
	}

	// producer from now on
	printf("I am the producer now.\n");
	
	it = 0;
	while(1) {
		usleep(20003);

		char* mesg;
		int shm_key = shmget(IPC_PRIVATE, 100*sizeof(*mesg), IPC_CREAT | 0600);
		mesg = shmat(shm_key, 0, 0);
		snprintf(mesg, 100, "foo %u", it);
		mesg = 0;

		int sent_ok = QSend(it++% MAX_PRIORITY, shm_key);
		printf("sent %s [%d]\n", sent_ok ? "ok" : "failed", shm_key);
		printf("SEND ");
		QPrintQueue();

		shmdt(mesg);
	}
}

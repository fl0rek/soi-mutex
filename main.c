#include "queue.h"
#include <stdio.h>
#include <unistd.h>

typedef void(* consumer_function)(unsigned);

#define CONSUMER_NUMBER 100
consumer_function consumer[CONSUMER_NUMBER] = { 0 };

void basic_read(unsigned myid) {
	printf("consumer %u\n", myid);
	while(1) {
		printf("%u: %s\n", myid, QRec());
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
		printf("%x, %u", consumer[it], it);
		if(fork() == 0) { // consumer number $it
			consumer[it](it);
		}
		it++;
	}

	// producer from now on
	printf("I am the producer now.\n");
	
	it = 9;
	char* mesg = "foo";
	while(1) {
		usleep(20003);
		int sent_ok = QSend(it++% MAX_PRIORITY, mesg);
		printf("sent %s\n", sent_ok ? "ok" : "failed");
	}
}

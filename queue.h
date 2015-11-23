#pragma once

#include <sys/ipc.h>
#include <sys/shm.h>

#define BUFF_LEN 8
#define MAX_PRIORITY 10

#define QPos(x) (x&(BUFF_LEN-1))


int QSend(unsigned priority, int enq);
int QRec(void);
void QInit(void);
void QPrintQueue(void);

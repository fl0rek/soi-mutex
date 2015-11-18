#pragma once

#define BUFF_LEN 128
#define MAX_PRIORITY 10


int QSend(unsigned priority, char* enq);
char* QRec(void);
void QInit(void);

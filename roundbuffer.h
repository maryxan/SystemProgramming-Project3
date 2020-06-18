#ifndef ROUNDBUFFER_H
#define ROUNDBUFFER_H

typedef struct roundBuffer
{
	int numThreads;
	int fd;
	
	int pos;
	int max;
	int head;
    int tail;
	int *buffer;
		
}roundBuffer;


int push(roundBuffer *roundbuffer, int data);
int pop(roundBuffer *roundbuffer, int data);
void print(roundBuffer* roundbuffer);

#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "roundbuffer.h"

// typedef struct roundBuffer
// {
// 	int pos;
// 	int max;
// 	int head;
//  int tail;
// 	int *buffer;
		
// }roundBuffer;


int push(roundBuffer *roundbuffer, int data)
{

    int next;

    next = roundbuffer->head + 1;  // next is where head will point to after this write.
    if (next >= roundbuffer->max)
        next = 0;

    if (next == roundbuffer->tail){
    	printf("roundBuffer is full\n");
    	 return -1;

    }  // if the head + 1 == tail, circular buffer is full
       
    printf("pushing fd %d \n",data);
    roundbuffer->buffer[roundbuffer->head] = data;  
    roundbuffer->head = next;             // head to next data offset.

    printf("head %d tail %d\n",roundbuffer->head , roundbuffer->tail);

    return 0;  
}
int pop(roundBuffer *roundbuffer, int data)
{
    int next;

    if (roundbuffer->head == roundbuffer->tail){
    	printf("roundBuffer is empty\n");
    	//return -1;
    }  // if the head == tail, we don't have any data
        

    next = roundbuffer->tail + 1;  // next is where tail will point to after this read.
    if(next >= roundbuffer->max)
        next = 0;

    data = roundbuffer->buffer[roundbuffer->tail]; 
    roundbuffer->tail = next;              // tail to next offset.

    printf("DATA FROM POP %d\n",data );

    return data;  
}
void print(roundBuffer* roundbuffer)
{
    printf("PRINTING ROUND BUFFER!\n");
    for(int i = 0; i < roundbuffer->max; i++)
    {
        if(roundbuffer->buffer[i] == 0)
        {
            printf("%d: NULL\n", i);
        }
        else
        {
            printf("%d: %d\n", i, roundbuffer->buffer[i]);
        }
    }
}
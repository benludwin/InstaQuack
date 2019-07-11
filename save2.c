#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "bounded_queue.h"

#define QUACKSIZE 20
#define CAPTIONSIZE 20
#define MAXTOPICS 20
#define DELTA 20

struct topic_entry{
	int entrynum;			//entry number based at 1
	struct timeval timestamp;	//timestamp of enqueue

	int pubID;			//publisher ID	//init in put from publisher
	char photoUrl[QUACKSIZE];	//photoUrl  		
	char photoCaption[CAPTIONSIZE];	//photoCaption
};

struct bounded_queue
{
        int size;       // capacity
        struct topic_entry **buffer;  // storage
        long long head; // 
        long long tail; //
};

int RoundIDToBufferIndex(int size, long long index)
{
        long long value = (index % ((long long)size));
        return (int)value;
}

BoundedQueue *BB_MallocBoundedQueue(long size)
{
	struct bounded_queue *returnValue = NULL;
	returnValue = (BoundedQueue *)malloc(sizeof(BoundedQueue));
	returnValue->buffer = (struct topic_entry **)malloc(sizeof(struct topic_entry *) * size);
	for (int i = 0; i < size; i++){
		returnValue->buffer[i] = NULL;
	}
	returnValue->head = returnValue->tail = 0;
	//returnValue->size = size;
	returnValue->size = size-1;
        return (BoundedQueue *)returnValue;
}

long long BB_TryEnqueue(struct bounded_queue *queue,struct topic_entry *item)
{
        long long returnValue = 0;
        if (BB_IsFull(queue) == 0){
		queue->buffer[RoundIDToBufferIndex(queue->size, queue->head)] = item;
		returnValue = queue->head;
		gettimeofday(&item->timestamp, NULL);
		queue->head += 1;
		item->entrynum = queue->head;
	}
	return returnValue;
}

int GetEntry(int lastentry, struct topic_entry *t, struct bounded_queue *queue){
	struct topic_entry *topic;	
	if (BB_IsEmpty(queue) == 1){
		return 0;
	}
	for (int i = 1; BB_IsIdValid(queue, lastentry + i) == 1;i++){
		topic = queue->buffer[RoundIDToBufferIndex(queue->size, lastentry+i)];
		if (topic != NULL && i == 1){
			t = topic;
			return 1;
		} else if (topic !=NULL){
			t = topic;
			return t->entrynum;
		}
	}
}
int BB_IsIdValid(struct bounded_queue *queue,long long id)
{
	int returnValue = 0;  
 	if ((BB_IsEmpty(queue) == 0) && (queue->tail <= id) && (id < queue->head)){
		returnValue = 1;
	}
 	return returnValue;
}

void Dequeue(struct bounded_queue *queue){
	for (int i = queue->head-1; BB_IsIdValid(queue, i) == 1;i--){
		if (queue->buffer[RoundIDToBufferIndex(queue->size, i)]->timestamp.tv_sec > DELTA){
			queue->buffer[RoundIDToBufferIndex(queue->size, i)] = NULL;
			//queue->head -= 1;
			//if they are too old, set them
		}
	}
}

int BB_TryDequeue(struct bounded_queue *queue,long long id)
{
        int  returnValue = 0;
        if (BB_IsEmpty(queue) == 0 && BB_IsIdValid(queue, id) && id == queue->tail){
		queue->buffer[RoundIDToBufferIndex(queue->size, id)] = NULL;
		queue->tail += 1;
		returnValue = 1;
	}
	return returnValue;
}

long long BB_GetFront(struct bounded_queue *queue)
{
        long long returnValue = -1;
        if(!BB_IsEmpty(queue) && queue->head != queue->tail)
        {
                returnValue = queue->head-1;
        }
        return returnValue;
}

long long BB_GetBack(struct bounded_queue *queue)
{
        long long returnValue = -1;
	if (BB_IsEmpty(queue) == 0){
		returnValue = queue->tail;
	}
        return returnValue;
}

int BB_GetCount(struct bounded_queue *queue)
{
        long long returnValue = queue->head - queue->tail;
        return (int)returnValue;
}



struct topic_entry *BB_GetItem(struct bounded_queue *queue,long long id)
{
        struct topic_entry *returnValue = NULL;
	int index;
	if (BB_IsIdValid(queue, id) == 1){
		index = RoundIDToBufferIndex(queue->size, id);
		returnValue = queue->buffer[index];
	}
        return returnValue;
}

int BB_IsFull(struct bounded_queue *queue)
{
	int returnValue = 0;
 	if ((int)queue->head - (int)queue->tail == (int)queue->size){
		returnValue = 1;
	}
 	return returnValue;
}

int BB_IsEmpty(struct bounded_queue *queue)
{
	int returnValue = 0;
	if (queue->head == queue->tail){
		returnValue = 1;	
	}
        return returnValue;
}

void BB_FreeBoundedQueue(struct bounded_queue *queue)
{
	for (int i = 0; i < queue->size; i++){
		free(queue->buffer[i]);
	}
	free(queue->buffer);
	free(queue);
}

#ifndef BOUNDED_QUEUE_H
#define BOUNDED_QUEUE_H
#define QUACKSIZE 64
#define CAPTIONSIZE 64

typedef struct bounded_queue BoundedQueue;

typedef struct topic_entry TopicEntry;

BoundedQueue *BB_MallocBoundedQueue(long size);

long long BB_TryEnqueue(BoundedQueue *queue, void *item); 

int BB_TryDequeue(BoundedQueue *queue,long long id);

long long BB_GetFront(BoundedQueue *queue);

long long BB_GetBack(BoundedQueue *queue);

int BB_GetCount(BoundedQueue *queue);

int BB_IsIdValid(BoundedQueue *queue,long long id);

void *BB_GetItem(BoundedQueue *queue,long long id);

int BB_IsFull(BoundedQueue *queue);

int BB_IsEmpty(BoundedQueue *queue);

void BB_FreeBoundedQueue(BoundedQueue *queue);
struct topic_entry{
	int entrynum;			//entry number based at 1
	struct timeval timestamp;	//timestamp of enqueue


	int topicID;
	int pubID;			//publisher ID	//init in put from publisher
	char photoUrl[QUACKSIZE];	//photoUrl  		
	char photoCaption[CAPTIONSIZE];	//photoCaption
};

struct bounded_queue
{
        long size;       // capacity
        void **buffer;  // storage
        int head; // 
        int tail; //
};


#endif

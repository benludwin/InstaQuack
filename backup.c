#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/time.h>
#include <errno.h>
#include "bounded_queue.h"
#include "thread_safe_bounded_queue.h"

#define NUMPROXIES 3
#define MAXTOPICS 10
#define MAXENTRIES 20
#define DELTA 2

struct Producer{
	pthread_t tid;
	int free;
	sem_t sem;
	struct thread_safe_bounded_queue **TQ_list;
};

struct Consumer{
	pthread_t tid;
	int free;
	sem_t sem;
	struct thread_safe_bounded_queue **TQ_list;
};

struct CleanUp{
	pthread_t tid;
	int free;
	sem_t sem;
	struct thread_safe_bounded_queue **TQ_list;
};

void *producerRoutine(void *i){
	//wait on semaphore
	//enqueue
	//sleep
	int *i_ptr = (int *)i;
	fprintf(stderr, "producer %d\n", *i_ptr);
	return NULL;
}

void *consumerRoutine(void *i){
	//wait on semaphore
	//getentry - put
	//sleep	
	fprintf(stderr, "consumer\n");
	return NULL;
}

void *cleanupRoutine(void *i){
	//wait on semaphore//dequeue - get
	//sleep
	return NULL;
}


void *Publisher(void * TopicQueue){
	struct thread_safe_bounded_queue *TQ = (struct thread_safe_bounded_queue *)TopicQueue;
	FILE *input;
	char *command = "./producer_input";
	input = fopen(command, "r");
	char line[64];
	
	while (fgets(line, sizeof(line), input) != NULL){
		int sleep_time;
		struct topic_entry *current_entry = (struct topic_entry *)malloc(sizeof(struct topic_entry));
		current_entry->topicID = atoi(line);
		
		fgets(line, sizeof(line), input);
		strcpy(current_entry->photoUrl, line);
		
		fgets(line, sizeof(line), input);
		strcpy(current_entry->photoCaption, line);
		
		fgets(line, sizeof(line), input);
		sleep_time = atoi(line);
		
		int test = TS_BB_TryEnqueue(TQ, current_entry);		
		if (test == -1){
			fprintf(stderr, "enqueue failed\n");
		}
		sleep(sleep_time);
	}
	return NULL;	
}

void *Subscriber(void * TopicQueue){
	struct thread_safe_bounded_queue *TQ = (struct thread_safe_bounded_queue *)TopicQueue;
	FILE *input;
	char *command = "./consumer_input";
	input = fopen(command, "r");
	char line[64];
	
	while (fgets(line, sizeof(line), input) != NULL){
		int sleep_time;
		int topicID;

		topicID = atoi(line);
		fgets(line, sizeof(line), input);
		sleep_time = atoi(line);

		TS_BB_TryDequeue(TQ, topicID);
		sleep(sleep_time);
	}	
	return NULL;	
}

void Dequeue(struct thread_safe_bounded_queue *TQ){
        struct timeval time;
        gettimeofday(&time, NULL);
	int age;
	for (;;){
		struct topic_entry *topic_entry = TS_BB_GetItem(TQ, TS_BB_GetBack(TQ));
		age = abs(topic_entry->timestamp.tv_sec - time.tv_sec);
		if (age > DELTA){
                	TS_BB_TryDequeue(TQ, TS_BB_GetBack(TQ));
		}else{
			return;
		}
	}
}


void *CleanUp(void * CleanUpPtr){

	struct CleanUp *cleanup = (struct CleanUp *)CleanUpPtr;

	while (1){
		for (int i = 0; i < MAXTOPICS; i++){
			if (TS_BB_IsEmpty(cleanup->TQ_list[i]) == 0){
				fprintf(stderr, "Cleaning up TopicQueue # %d\n", i);
				Dequeue(cleanup->TQ_list[i]);
			}
		}
		int sched_yield(void);
		sleep(5);
	}

	return NULL;	
}

int main(int argc, char *argv[]){
	struct thread_safe_bounded_queue **topics = (struct thread_safe_bounded_queue **)malloc(sizeof(struct thread_safe_bounded_queue *) * MAXTOPICS);
	
	struct Producer **prods = (struct Producer **)malloc(sizeof(struct Producer *));
	struct Consumer **cons = (struct Consumer **)malloc(sizeof(struct Consumer *));
	
	struct CleanUp *cleanup = (struct CleanUp *)malloc(sizeof(struct CleanUp));
	cleanup->TQ_list = topics;

	for (int i = 0; i < MAXTOPICS; i++){
		topics[i] = TS_BB_MallocBoundedQueue((long int)MAXENTRIES);
	}
	for (int i = 0; i < NUMPROXIES; i++){
		prods[i] = (struct Producer *)malloc(sizeof(struct Producer));
		prods[i]->TQ_list = topics;
		cons[i] = (struct Consumer *)malloc(sizeof(struct Consumer));
		cons[i]->TQ_list = topics;
	}

	pthread_create(&cleanup->tid, NULL, CleanUp, cleanup);
		
	for (int i = 0; i < NUMPROXIES; i++){
		pthread_create(&prods[i]->tid, NULL, Publisher, topics[0]);
	//	pthread_create(&cons[i]->tid, NULL, Subscriber, topics[0]);
	}
	for (int i = 0; i < NUMPROXIES; i++){
	//	pthread_join(cons[i]->tid, NULL);
		pthread_join(prods[i]->tid, NULL);
	}

	pthread_join(cleanup->tid, NULL);

	return 0;
	/*
	for (int i = 0; i < NUMPROXIES; i++){
		prods[i].free = 1;
		sem_init(&prods[i].sem, 0, 0);
		cons[i].free = 1;
		sem_init(&cons[i].sem, 0, 0);
	}
	*/
	return 1;
}

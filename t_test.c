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
	int lastentry_list[MAXTOPICS];
	struct thread_safe_bounded_queue **TQ_list;
};

struct CleanUp{
	pthread_t tid;
	int free;
	sem_t sem;
	struct thread_safe_bounded_queue **TQ_list;
};

void *Publisher(void * ProducerPtr){
	struct Producer *publisher = (struct Producer *)ProducerPtr;

	FILE *input;
	char *command = "./producer_input";
	input = fopen(command, "r");
	char line[64];
	
	while (fgets(line, sizeof(line), input) != NULL){
		int sleep_time;
		int enqueue_test;
		struct topic_entry *current_entry = (struct topic_entry *)malloc(sizeof(struct topic_entry));
		current_entry->entrynum = -1;
		current_entry->timestamp = (struct timeval){0};
		current_entry->topicID = atoi(line);
		
		fgets(line, sizeof(line), input);
		strcpy(current_entry->photoUrl, line);
		
		fgets(line, sizeof(line), input);
		strcpy(current_entry->photoCaption, line);
		
		fgets(line, sizeof(line), input);
		sleep_time = atoi(line);
		
		
		enqueue_test = 1;
		enqueue_test = TS_BB_TryEnqueue(publisher->TQ_list[current_entry->topicID-1], current_entry);
		while (enqueue_test == -1){
			fprintf(stderr, "Queue is full, yielding scheduler and trying again later\n");
			int sched_yield(void);
			enqueue_test = TS_BB_TryEnqueue(publisher->TQ_list[current_entry->topicID-1], current_entry);
		
		}
		current_entry->entrynum = enqueue_test+1;
		sleep(sleep_time);
	}
	fclose(input);
	return NULL;	
}

int getEntry(int lastentry, struct topic_entry **t, struct thread_safe_bounded_queue *TQ){
	struct topic_entry *topic = NULL;
	if (TS_BB_IsEmpty(TQ) == 1){
		return 0;
	}
	for (int i = TS_BB_GetBack(TQ); i <= TS_BB_GetFront(TQ); i++){
		topic = (struct topic_entry *)TS_BB_GetItem(TQ, i);
		if (topic->entrynum == lastentry+1){
			*t = topic;
			return 1;
		}
	}
	for (int i = TS_BB_GetBack(TQ); i <= TS_BB_GetFront(TQ); i++){
		topic = TS_BB_GetItem(TQ, i);
		if (topic->entrynum > lastentry+1){
			*t = topic;
			return topic->entrynum;
		}
	}	
	return 0;
}

void *Subscriber(void * ConsumerPtr){
	struct Consumer *consumer = (struct Consumer *)ConsumerPtr;

	FILE *input;
	char *command = "./consumer_input";
	input = fopen(command, "r");
	char line[64];

	struct topic_entry *current_topic = NULL;
	while (fgets(line, sizeof(line), input) != NULL){
		int sleep_time;
		int topicID;
		int success;
		topicID = atoi(line);
		fgets(line, sizeof(line), input);
		sleep_time = atoi(line);
		success = getEntry(consumer->lastentry_list[topicID-1], &current_topic, consumer->TQ_list[topicID-1]);
		if (success == 0){
			fprintf(stderr, "getEntry failed\n");
		} else if (success == 1){
			fprintf(stderr, "getEntry succeded on lastentry+1\n");
			fprintf(stderr, "%s", current_topic->photoCaption);
		} else {
			consumer->lastentry_list[topicID] = success;
			fprintf(stderr, "getEntry succeded with lastentry updated to %d\n", success);
			fprintf(stderr, "%s", current_topic->photoCaption);
		}
		sleep(sleep_time);
	}
	fclose(input);	
	return NULL;	
}

void Dequeue(struct thread_safe_bounded_queue *TQ){
        struct timeval time;
	time = (struct timeval){0};
        gettimeofday(&time, NULL);
	int age = 0;
	for (;;){
		if (TS_BB_GetBack(TQ) == -1){
			return;
		}
		struct topic_entry *topic_entry = TS_BB_GetItem(TQ, TS_BB_GetBack(TQ));
		age = abs(topic_entry->timestamp.tv_sec - time.tv_sec);
		if (age > DELTA){
                	int success = TS_BB_TryDequeue(TQ, TS_BB_GetBack(TQ));
			if (success == 1){
				free(topic_entry);
			}
		}else{
			return;
		}
	}
}

void *CleanUp(void * CleanUpPtr){
	struct CleanUp *cleanup = (struct CleanUp *)CleanUpPtr;
	int empty_runs = 0;
	int all_empty;
	while (1){
		all_empty = 1;
		for (int i = 0; i < MAXTOPICS; i++){
			if (TS_BB_IsEmpty(cleanup->TQ_list[i]) == 0){
				fprintf(stderr, "Cleaning up TopicQueue # %d\n", i);
				Dequeue(cleanup->TQ_list[i]);
				all_empty = 0;
			}
		}
		if (all_empty == 1){
			empty_runs++;
			fprintf(stderr, "All TQ's empty\n");
		} else {
			empty_runs = 0;
		}
		if (empty_runs >= 3){
			fprintf(stderr, "Three consecutive attempts to Dequeue with all TQ's empty, exiting CleanUp.\n");
			return NULL;
		}
		sleep(3);
		
		int sched_yield(void);
	}
	return NULL;	
}

int main(int argc, char *argv[]){
	struct thread_safe_bounded_queue **topics = (struct thread_safe_bounded_queue **)malloc(sizeof(struct thread_safe_bounded_queue *) * MAXTOPICS);
	
	struct Producer **prods = (struct Producer **)malloc(sizeof(struct Producer *) * NUMPROXIES);
	struct Consumer **cons = (struct Consumer **)malloc(sizeof(struct Consumer *) * NUMPROXIES);
	
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
		for (int j = 0; j < MAXTOPICS; j++){
			cons[i]->lastentry_list[j] = 0;
		}
	}

	pthread_create(&cleanup->tid, NULL, CleanUp, cleanup);
		
	for (int i = 0; i < NUMPROXIES; i++){
		pthread_create(&prods[i]->tid, NULL, Publisher, prods[i]);
		pthread_create(&cons[i]->tid, NULL, Subscriber, cons[i]);
	}
	for (int i = 0; i < NUMPROXIES; i++){
		pthread_join(prods[i]->tid, NULL);
		pthread_join(cons[i]->tid, NULL);
	}

	pthread_join(cleanup->tid, NULL);
	
	for (int i =0; i < NUMPROXIES; i++){
		free(prods[i]);
		free(cons[i]);
	}
	for (int i = 0; i < MAXTOPICS; i++){
		TS_BB_FreeBoundedQueue(topics[i]);
	}
	free(topics);
	free(prods);
	free(cons);
	free(cleanup);
	return 0;
}

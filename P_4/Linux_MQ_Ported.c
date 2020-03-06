/*
Exercise 3 Problem 4
Message queue based off heap_mq and posix_mq ported to Linux
Author: Mike Fruge
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* POSIX Libraries */
#include <semaphore.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>

/* Linux Libraries */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>





/* Message queue */
#include "mqueue.h"
#include <sys/stat.h>
#include <fcntl.h>


/* Defines */
#define ERROR 				-1
#define MAX_MSG_SIZE 		128
#define MSG_LIMIT			100
#define MSG_QUEUE_NAME 		"/thismsg_Q"

#define PERMISSIONS			0666


/* Globals */

	/* Threads, thread attributes, scheduling structures */
pthread_t ptSender, ptReceiver;
pthread_attr_t ptatrSender, ptatrReceiver;
struct sched_param schedMain, schedSender, schedReceiver;
pid_t thisProcess;


	/* Message Queue */

struct mq_attr queue_attr;


static char canned_msg[] = "this is a test, and only a test, in the event of a real emergency, you would be instructed ...";


/* Functions */

void* receiver(void * Param)
{
	printf(" Reciever Ready \n");
	mqd_t mymq;
  	char buffer[MAX_MSG_SIZE];
  	int prio;
  	int nbytes;

  	/* note that VxWorks does not deal with permissions? */
  	mymq = mq_open(MSG_QUEUE_NAME, O_CREAT|O_RDWR, 0, &queue_attr);

  	if(mymq == (mqd_t)ERROR)
    	perror("mq_open");

  	/* read oldest, highest priority msg from the message queue */
  	if((nbytes = mq_receive(mymq, buffer, MAX_MSG_SIZE, &prio)) == ERROR)
  	{
    	perror("mq_receive");
  	}
  	else
  	{
    	buffer[nbytes] = '\0';
    	printf("receive: msg %s received with priority = %d, length = %d\n", buffer, prio, nbytes);
  	}
}



void* sender(void * Param)
{
	printf(" Sender Ready \n");
	mqd_t mymq;
	int prio;
	int nbytes;

	/* note that VxWorks does not deal with permissions? */
	mymq = mq_open(MSG_QUEUE_NAME, O_RDWR, 0, &queue_attr);

	if(mymq == (mqd_t)ERROR)
		perror("mq_open");

	/* send message with priority=30 */
	if((nbytes = mq_send(mymq, canned_msg, sizeof(canned_msg), 30)) == ERROR)
	{
		perror("mq_send");
	}
	else
	{
		printf("send: message successfully sent\n");
	}
}


int main()
{
	int maxPrio;
	/* Set queue attributes */
	queue_attr.mq_maxmsg = MSG_LIMIT;
	queue_attr.mq_msgsize = MAX_MSG_SIZE;
	queue_attr.mq_flags = 0;

	/* Set main scheduling policy */

	thisProcess = getpid();

	/* Get main scheduling policy */
	maxPrio = sched_get_priority_max(SCHED_FIFO);
	if(sched_getparam(thisProcess, &schedMain) == EINVAL)
	{
		perror("Get Main Scheduling Parameters");
	}

	/* Set priority of main to be highest */
	printf("%d\n", maxPrio);
	schedMain.sched_priority = maxPrio;

	/* Set main scheduling policy */
	if(sched_setscheduler(0, SCHED_FIFO, &schedMain) == ERROR)
	{
		perror("Set Main scheduling policy");
	}


	/* Init Thread attribute structures */

	if(pthread_attr_init(&ptatrSender) == ERROR)
	{
		perror(" Creating Sender Attribute ");
	}

	if(pthread_attr_init(&ptatrReceiver) == ERROR)
	{
		perror(" Creating Receiver Attribute ");
	}

	/* Set scheduling to be declared explicitly rather than inherited */

	if(pthread_attr_setinheritsched(&ptatrSender, PTHREAD_EXPLICIT_SCHED) == ERROR)
	{
		perror(" Set Sender Inheritance ");
	}
	
	if(pthread_attr_setinheritsched(&ptatrReceiver, PTHREAD_EXPLICIT_SCHED) == ERROR)
	{
		perror(" Set Receiver Inheritance ");
	}

	/* Set policy to FIFO */

	if(pthread_attr_setschedpolicy(&ptatrSender, SCHED_FIFO) == ERROR)
	{
		perror(" Set Sender Scheduling Policy ");
	}
	
	if(pthread_attr_setschedpolicy(&ptatrReceiver, SCHED_FIFO) == ERROR)
	{
		perror(" Set Receiver Scheduling Policy ");
	}

	/* Adjust priorities to effectivley mirror heap_mq.c */

		/* Receiver has 2nd highest priority */
	schedReceiver.sched_priority = maxPrio - 1;

		/* Sender has 3rd highest priority */
	schedSender.sched_priority = maxPrio - 2;
	
	/* Set the priorities */
	pthread_attr_setschedparam(&ptatrReceiver, &schedReceiver);
	pthread_attr_setschedparam(&ptatrSender, &schedSender);


	/* Create threads */

	printf("	CREATING 	THREADS 	\n");

	if( pthread_create(&ptReceiver, &ptatrReceiver, receiver, NULL) == ERROR)
	{
		perror(" Creating Receiver Thread");
		exit(1);
	}
	
	if( pthread_create(&ptSender, &ptatrSender, sender, NULL) == ERROR)
	{
		perror(" Creating Sender Thread");
		exit(1);bbbb
	}


	/* Wait for threads to finish their transactions */

	if(pthread_join(ptSender, NULL))
	{
		perror(" Sender Join with Main ");
	}

	if(pthread_join(ptReceiver, NULL))
	{
		perror(" Receiver Join with Main ");
	}


	printf(" End of Main Thread\n");
	return 0;
}
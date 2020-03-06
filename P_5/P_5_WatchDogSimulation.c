/*************************************************************************************************************************************
 * 													EXERCISE 3 RTES
 * 									WATCHDOG TIMER CONCEPTS SIMULATED THRU TIMED SEMAPHORE
 * 										NAME: AAKSHA JAYWANT & MICHAEL FRUGE
 * 				REFERENCES: https://www.geeksforgeeks.org/rand-and-srand-in-ccpp/
**************************************************************************************************************************************/
/************ HEADERS ***************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>


/************ DEFINES ***************/

#define TIMED_MUTEX



/****** TIMESPEC & ATTITUDE STATE STRUCTURE *********/
typedef struct states
{
	struct timespec time_stamp;
	double x,y,z, roll, pitch, yaw;
}threadParams;

pthread_mutex_t resource_LOCK;
pthread_t thread1, thread2;
pthread_attr_t at_1, at_2, at_3;
threadParams threadParameter;
int i=0, j=0, flag=0;

/**
  * @brief: This thread1 updates the elements of the timespec and attitude state structure
  *
  * @param : 
  *
  * @return status: None
  */

void *update_val(void *threadp)											
{
	struct timespec timeout;

	while(i<5)
	{	
	// if(flag == 0)
	// {
		/* Get current time */
		clock_gettime(CLOCK_REALTIME, & timeout);

		timeout.tv_sec += 3;
		int fd = pthread_mutex_timedlock(&resource_LOCK, &timeout);									//mutex in locked state
		
		if(fd == ETIMEDOUT) printf("No new data available at %lu seconds, %lu nanoseconds\n", timeout.tv_sec, timeout.tv_nsec);

		else if(fd == 0)
		{
			threadParams* threadParameter = (threadParams*) threadp;
			threadParameter->x = (double)rand();
			threadParameter->y = (double)rand();
			threadParameter->z = (double)rand();
			threadParameter->roll = (double)rand();
			threadParameter->pitch = (double)rand();
			threadParameter->yaw = (double)rand();
			
			clock_gettime(CLOCK_REALTIME,&(threadParameter->time_stamp));
			
			printf("############################################## THREAD A UPDATES #############################################################################\n");
			printf("Timestamp-> %lu sec: %lu nsec\n",(threadParameter->time_stamp).tv_sec,(threadParameter->time_stamp).tv_nsec);
			printf("Updated Values-> x=%f, y=%f, z=%f, roll=%f, pitch=%f, yaw=%f\n",threadParameter->x,threadParameter->y,threadParameter->z,threadParameter->roll,threadParameter->pitch,threadParameter->yaw);
			printf("_____________________________________________________________________________________________________________________________________________\n");
		
			fd = pthread_mutex_unlock(&resource_LOCK);
		}

		i++;
		// flag = 1;
		// }
	}


	pthread_exit(NULL);
}

/**
  * @brief: This thread2 reads the elements of the timespec and attitude state structure
  *
  * @param : 
  *
  * @return status: None
  */


#ifndef TIMED_MUTEX

void *read_val(void *threadp)
{
	while(j<5)
	{
	if(flag == 1)
	{
	sleep (5);
	threadParams* threadParameter = (threadParams*) threadp;
	pthread_mutex_lock(&resource_LOCK);
	printf("############################################## THREAD B READING FROM UPDATED STRUCT #######################################################\n");
	printf("Timestamp-> %lu sec: %lu nsec\n",(threadParameter->time_stamp).tv_sec,(threadParameter->time_stamp).tv_nsec);
	printf("Updated Values-> x=%f, y=%f, z=%f, roll=%f, pitch=%f, yaw=%f\n",threadParameter->x,threadParameter->y,threadParameter->z,threadParameter->roll,threadParameter->pitch,threadParameter->yaw);
	printf("___________________________________________________________________________________________________________________________________________\n\n\n\n");
	pthread_mutex_unlock(&resource_LOCK);
	j++;
	flag = 0;
	}
	}
	pthread_exit(NULL);
}

#endif

int main(void)
{
	int mutex_RET;
	srand(time(0));											//Current time as seed for random generator
	mutex_RET = pthread_mutex_init(&resource_LOCK, NULL);	//initialize mutex
	if(mutex_RET != 0)
		printf("Mutex initialization not successful");


	/* Lock mutex from main so thread must wait and time out */
	pthread_mutex_lock(&resource_LOCK);

	pthread_attr_init(&at_1);

#ifndef TIMED_MUTEX					
	pthread_attr_init(&at_2);
#endif


	pthread_create(&thread1, 
					&at_1,
            		update_val, 
					(void *)&(threadParameter));

#ifndef TIMED_MUTEX
	pthread_create(&thread2, 
					&at_2,
            		read_val, 
					(void *)&(threadParameter));
#endif

	pthread_join(thread1, NULL);

#ifndef TIMED_MUTEX
	pthread_join(thread2, NULL);
#endif

	pthread_mutex_destroy(&resource_LOCK);						//destroy mutex

	return 0;
}






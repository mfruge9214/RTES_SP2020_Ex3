#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


#define NUM_THREADS 2
#define THREAD_1 1
#define THREAD_2 2


#define MAX_BACKOFF   1000000     // 1E6
#define NSEC_MAX      1000000000  // 1E9

typedef struct
{
    int threadIdx;
} threadParams_t;



pthread_t threads[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];

struct sched_param nrt_param;

pthread_mutex_t rsrcA, rsrcB;

volatile int rsrcACnt=0, rsrcBCnt=0, noWait=0;


volatile int thread1_done, thread2_done;


void *grabRsrcs(void *threadp)
{
   int lockRet;
   threadParams_t *threadParams = (threadParams_t *)threadp;
   int threadIdx = threadParams->threadIdx;
   
   /* Structures for Random Back-off Scheme */
   struct timespec delayTime;

   /* Obtain random number between 5 and 1E6 to set jitter for backoff */
   int jitter = (rand() % MAX_BACKOFF) + 5;

   if(threadIdx == THREAD_1)
   {
     clock_gettime(CLOCK_REALTIME, &delayTime);

     /* Use jitter to calculate wait time */
     if(  ((long) jitter + delayTime.tv_nsec) >=  NSEC_MAX)
     {
        delayTime.tv_sec++;
        delayTime.tv_nsec = 1;
     }

     printf("THREAD 1 waiting for recources \n");

     /* Try to grab A */
     lockRet = pthread_mutex_timedlock(&rsrcA, &delayTime);

     if( lockRet == 0)
     {
        rsrcACnt++;
        printf("THREAD 1 got A \n");
        if(!noWait) usleep(1000000);
        printf(" THREAD 1 trying for B \n");

        /* Try to grab B*/
        lockRet = pthread_mutex_timedlock(&rsrcB, &delayTime);
        
        if( lockRet == 0)
        {
          rsrcBCnt++;
          printf("THREAD 1 got A and B\n");
        }

        else if (lockRet == ETIMEDOUT)
        {
          /* Release Held Resources */

          pthread_mutex_unlock(&rsrcA);
          printf(" THREAD 1 Timed Out waiting for B \n");
          pthread_exit(NULL);
        }
     }

     else if(lockRet == ETIMEDOUT)
     {
        printf(" THREAD 1 Timed Out waiting for A \n");
        pthread_exit(NULL);
     }
     
     
     
     /* If we got both resources, unlock them */
     pthread_mutex_unlock(&rsrcB);
     pthread_mutex_unlock(&rsrcA);

     /* Set flag */
     thread1_done = 1;
     printf("THREAD 1 done\n");
   }
   else
   {
     clock_gettime(CLOCK_REALTIME, &delayTime);

     /* Use jitter to calculate wait time */
     if(  ((long) jitter + delayTime.tv_nsec) >=  NSEC_MAX)
     {
        delayTime.tv_sec++;
        delayTime.tv_nsec = 1;
     }

     printf("THREAD 2 waiting for resources\n");

     lockRet = pthread_mutex_timedlock(&rsrcB, &delayTime);

     if( lockRet == 0)
     {
        rsrcBCnt++;
         printf("THREAD 2 got B \n");
        if(!noWait) usleep(1000000);
        printf("THREAD 2 trying for A \n");

        lockRet = pthread_mutex_timedlock(&rsrcA, &delayTime);

        if( lockRet == 0)
        {
          rsrcACnt++;
          printf("THREAD 2 got B and A\n");
        }

        else if (lockRet == ETIMEDOUT)
        {
          pthread_mutex_unlock(&rsrcB);
          printf(" THREAD 2 Timed Out waiting for A \n");
          pthread_exit(NULL);          
        }
     }

     else if (lockRet == ETIMEDOUT)
     {
        printf(" THREAD 2 Timed Out waiting for B \n");
        pthread_exit(NULL);
     }
     
     pthread_mutex_unlock(&rsrcA);
     pthread_mutex_unlock(&rsrcB);

     /* Set flag */
     thread2_done = 1;
     printf("THREAD 2 done\n");

   }
   pthread_exit(NULL);
}



int main (int argc, char *argv[])
{
   int rc, safe=0;

   int i = 0;

   thread1_done = thread2_done = 0;

   // printf("Random Number: %d", rand());
   rsrcACnt=0, rsrcBCnt=0, noWait=0;

   if(argc < 2)
   {
     printf("Will set up unsafe deadlock scenario\n");
   }
   else if(argc == 2)
   {
     if(strncmp("safe", argv[1], 4) == 0)
       safe=1;
     else if(strncmp("race", argv[1], 4) == 0)
       noWait=1;
     else
       printf("Will set up unsafe deadlock scenario\n");
   }
   else
   {
     printf("Usage: deadlock [safe|race|unsafe]\n");
   }

   // Set default protocol for mutex
   pthread_mutex_init(&rsrcA, NULL);
   pthread_mutex_init(&rsrcB, NULL);

   printf("Creating thread %d\n", THREAD_1);
   threadParams[THREAD_1].threadIdx=THREAD_1;

   while(!thread1_done && !thread2_done && i<5)
   {
      if(!thread1_done)
      {
        rc = pthread_create(&threads[0], NULL, grabRsrcs, (void *)&threadParams[THREAD_1]);
        if (rc) {printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);}
          printf("Thread 1 spawned\n");

        if(safe) // Make sure Thread 1 finishes with both resources first
        {
          if(pthread_join(threads[0], NULL) == 0)
            printf("Thread 1: %x done\n", (unsigned int)threads[0]);
          else
            perror("Thread 1");
        }
      }

      if(!thread2_done)
      {
         printf("Creating thread %d\n", THREAD_2);
         threadParams[THREAD_2].threadIdx=THREAD_2;
         rc = pthread_create(&threads[1], NULL, grabRsrcs, (void *)&threadParams[THREAD_2]);
         if (rc) {printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);}
         printf("Thread 2 spawned\n");
      }
      
      printf("rsrcACnt=%d, rsrcBCnt=%d\n", rsrcACnt, rsrcBCnt);
      printf("will try to join CS threads unless they deadlock\n");

      if(!safe)
      {
        if(pthread_join(threads[0], NULL) == 0)
        {
          printf("Thread 1: %x done\n", (unsigned int)threads[0]);
          printf("rsrcACnt=%d, rsrcBCnt=%d\n", rsrcACnt, rsrcBCnt);
        }
        else
        {
          perror("Thread 1");
        }
      }

      if(pthread_join(threads[1], NULL) == 0)
      {
        printf("Thread 2: %x done\n", (unsigned int)threads[1]);
        printf("rsrcACnt=%d, rsrcBCnt=%d\n", rsrcACnt, rsrcBCnt);
      }

      else
      {
        perror("Thread 2");
      }

      i++;
    
    }   // END WHILE LOOP



   if(pthread_mutex_destroy(&rsrcA) != 0)
   {
     perror("mutex A destroy");
   }

   if(pthread_mutex_destroy(&rsrcB) != 0)
   {
     perror("mutex B destroy");
   }

  printf("All done\n");

   exit(0);
 }




/*    RANDOM   BACKOFF   SCHEME

- Change the 'lock' to 'timedlock' that uses a timespec through clock_gettime() and rand() and is added to the current time,
each will only block on a resource for a specified amount of time. once the timed_lock returns, we will release all resources obtained and exit the thread

- Threads will be created in a loop, and only when they set the flag THREAD_X_COMPLETE will the loop break and the program continue
struct timespec 
    


*/


/* pthread_mutex_lock -> pthread_mutex_trylock works to get the program running, although it looses its point */
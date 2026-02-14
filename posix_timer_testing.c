/*********************************************************
 * Simple posix timer testing with SIGEV_THREAD
 * The objective is to see if we create a timer useing timer_create and
 * specifiy the "struct sigevent *sevp" with the field "sigev_notify" as
 * SIGEV_THREAD, does the library create a whole new thread to receive to the
 * signal or the signal is received on the same thread where the timer is
 * created and armed
 * We do this as follows
 * - We create a regular mutex
 * - We setup the timer using SIGEV_THREAD such that it fires after
 *   EXPIRE_AFTER_ARM  seconds
 * - IN the main thread where the timer was setup, we  print a message right
 *   before locking the mutex 
 *   Lock the mitex 
 *   then sleep for MAIN_SLEEP_PERIOD secodns
 *   Unlock the mutex and log a message
 *   Sleep for another MAIN_SLEEP_PERIOD2 seconds
 *   exit(0)
 * - In the signal handler
 *   print a message right before locking the mutex
 *   lock the mutex
 *   See that there signal handler is stuck
 *   After the main thread unlocks the mutex, we verify that the handler is
 *   able to proceed
 *
 * How to see the effect if handler getting stuck
 * ------------------------------------------------
 * - As mentioned below in the comment above "is_timer_periodic", because we
 *   set the timer as "SIGEV_THREAD", every time the timer is armed (whether
 *   because it was made periodic by setting the field "it_interval" to
 *   non-zero OR by having the handler call timer_settime(), a new thread is
 *   created 
 * - Use "-p" option
 * - This will make the timer periodic with a the period TIMER_INTERVAL 
 *   which is made small
 * - Because the main thread locks the mutex then sleeps for MAIN_SLEEP_PERIOD
 *   seconds, the handler will get stuck behind the mutex
 * - A the same time, because the period "TIMER_INTERVAL" is small, the itmer
 *   will expire multiple times
 * - To see that many threads will be called do the following
 *   - Find the process ID by calling
 *        ps -ef | grep posix_timer_testing
 *   - List the threads by calling
 *       ps -T <pid>
 * - You will see many threads created
 * - You will also see these thread IDs in the priunt out of the function 
 *   thread_handler
 * 
 * 
 * If the above steps occur as specified above, then we know that the
 * SIGEV_THREAD creates a separate thread
 *
 * I got a good amount of code from
 * https://riptutorial.com/posix/example/16306/posix-timer-with-sigev-thread-notification
 *
 *
 * To the extent possible under law, I dedicate all copyright and related and
 * neighboring rights to the code snippets shown below to the public domain
 * worldwide; see CC0 Public Domain Dedication link at.
 * http://creativecommons.org/publicdomain/zero/1.0/
 * In other words, feel free to use
 * the code below in any way you wish, just don't blame me for any problems with
 * it not imply my endorsement to it or any part of it or any work
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR(s) BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


 * How to build
 *-------------
 * Use
 *    gcc -Wall -Wextra -g -O0 -o posix_timer_testing posix_timer_testing.c -lrt -pthread
 * Note: I used "-g -O0" to disable optimization so that that you get most of
 *       the info in the core dump in case it crashes
 *       Of course, you have to enable core dumping on your syste,
 *debug if it cars
 *****************************************************************/

#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h> /* for `struct sigevent` and SIGEV_THREAD */
#include <time.h>   /* for timer_create(), `struct itimerspec`,
                     * timer_t and CLOCK_REALTIME*/
#include <pthread.h>/* Needed for PTHREAD_ and pthread_ */

#include <syslog.h>
#include <stdint.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */

#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <sys/types.h>


#define EXPIRE_AFTER_ARM (1)
#define TIMER_INTERVAL (2) /* make small to show creation of new thread */
#define MAIN_SLEEP_PERIOD (5)
#define MAIN_SLEEP_PERIOD2 (30)
#define HANDLER_SLEEP_PERIOD (4)

/* To record the my thread */
static __thread long int my_thread;

/* the timer ID */
timer_t timerid;

/*
 * If true, then when calling "timer_settime()", we will set the field
 * "it_interval" to non-zero so that the timer is automatically reloaded every
 * time it expires and the interval would be "TIMER_INTERVAL"
 * REMEMBER 
 * - setting ""it_interval" field in "struct sigevent" when calling 
 *   timer_create results in creating a NEW thread when the timer is armed
 * - The timer is armed by calling timer_settime()
 *   - If the "it_interval" is set to non-zero, means the timer is periodic and
 *     hence it will automatically re-arm itself every time it expires, 
 *     - I.e. when calling timer_settime() a new thread is created
 *            every time the timer expires ANOTHER new thread is created
 * NOTE: V.I.
 * - setting ""it_interval" field to non-zero means the timer is periodic and
 *   hence it will re-arm itself everytime it expires
 * - Because we set sigevent.sigev_notify to "SIGEV_THREAD;", the posix
 *   library will fork a new thread every time is the timer is armed
 * - Hence if the thread(s) that received the signal(s) due to the previous
 *   re-arming(s) did not exit, this will result in increasing the number of
 *   threads every time the timer expires because it is automatically re-armed
 * - Hence if the handler has a bug where it gets stuck (e.g. on a mutex), this
 *   will cause a THREAD LEAK
 * 
 * If false, then the handler will re-arm the timer as follows
 * - The handler will call  "timer_settime()" 
 * - selt it_interval to zero and hence the timer will NOT be periodic
 * - set it_value to  TIMER_INTERVAL and hence the timer will expire after 
 */
static bool is_timer_periodic = false;


/* Mutex to see whether timer_create will create another thread */
static pthread_mutex_t one_thread_mutex = PTHREAD_MUTEX_INITIALIZER; __attribute__((unused))

void thread_handler(union sigval sv)
{
  uint32_t *num_calls_p;

   /* extract arg passed to use when calling timer_create()*/
  num_calls_p = sv.sival_ptr;
  
  /* Inc # of times handler is called */
  (*num_calls_p)++;

  /* Let me record my thread */
  my_thread = syscall(SYS_gettid);

  fprintf(stdout, "%s %d: my thread=%ld: called %u times. Going to lock mutex\n",
          __FUNCTION__, __LINE__,        
          my_thread, (*num_calls_p));

  pthread_mutex_lock(&one_thread_mutex);
  fprintf(stdout, "%s %d: my thread=%ld: "
          "LOCKED the mutex: going to sleep for %u seconds\n",
          __FUNCTION__, __LINE__,
          my_thread,
          HANDLER_SLEEP_PERIOD);
  
  sleep(HANDLER_SLEEP_PERIOD);
  
  pthread_mutex_unlock(&one_thread_mutex);
  fprintf(stdout, "%s %d: my thread=%ld: UNLOCKED the mutex\n",
          __FUNCTION__, __LINE__,
          my_thread);

  /*
   * If this is NOT a periodic timer, then we have to re-instate the timer
   * ourselves 
   */
  if (!is_timer_periodic) {
    struct itimerspec trigger;
    memset(&trigger, 0, sizeof(trigger));
    trigger.it_value.tv_sec = EXPIRE_AFTER_ARM;

    fprintf(stdout, "%s %d: my thread=%ld: "
            "Re-arming the timer to %lu seconds. \n",
            __FUNCTION__, __LINE__,
            my_thread,
            trigger.it_value.tv_sec);
    /* Arm the timer. No flags are set and no old_value will be retrieved.
     */
    if (0 > timer_settime(timerid, /* timer ID: 3rd arg in timer_create() */
                  0 /* flags */,
                  &trigger /* new trigger */, 
                          NULL /* OLD trigger */)) {
      fprintf(stderr, "%s %d: Cannot arm timer: %d %s\n",
              __FUNCTION__, __LINE__,
              errno, strerror(errno));
      return;
    }
  }
}

int
main (int		argc,
      char*		argv[])
{
  int c;
  /* Command line arguments */
  struct sigevent sev;
  struct itimerspec trigger;
  static uint32_t num_calls = 0;

  while ((c = getopt (argc, argv, "p")) != -1) {
    switch (c) {
    case 'p':
      is_timer_periodic = true;
      break;

    case 'h':
    default:
      if (c!= 'h') {
        printf("\nInvalid argument %c\n", c);
      }
      printf("\nUsage:\n");
      printf("%s [options]\n"
             "\t-p: Assign non-zero %u seconds to 'it_interval'\n",
             argv[0], TIMER_INTERVAL);
      exit(0);
    }
  }
             
  
  /* Let me record my thread */
  my_thread = syscall(SYS_gettid);
  
  /* Set all `sev` and `trigger` memory to 0 */
  memset(&sev, 0, sizeof(struct sigevent));
  memset(&trigger, 0, sizeof(struct itimerspec));
        
  /* 
   * Remember that "struct sigevent" specifies how the signal is delivered
   * The field ".sigev_notify" specifies the notification method. I.e. how a
   * signal is delivered  
   *
   * Depending on the value of ".sigev_notify", the remaining fields are used
   * ".sigev_notify" is SIGEV_THREAD:: I.e. Set the notification method as
   *   SIGEV_THREAD: 
   * Hence
   *  .sigev_notify_function specifies the siglan handler
   *  .sigev_value.sival_ptr contains an opaque pointer that the handler can
   *          access by dereferencing the field "sival_ptr"
   * Upon timer expiration, `sigev_notify_function` (thread_handler()),
   * will be invoked as if it were the start function of a new thread.
   */
  sev.sigev_notify = SIGEV_THREAD;
  sev.sigev_notify_function = &thread_handler;
  sev.sigev_value.sival_ptr = &num_calls;
  
  /* Create the timer. In this example, CLOCK_REALTIME is used as the
   * clock, meaning that we're using a system-wide real-time clock for
   * this timer.
   */
  if (0 < timer_create(CLOCK_REALTIME, &sev, &timerid)) {
    fprintf(stderr, "%s %d Cannot crete timer: %d %s\n",
            __FUNCTION__, __LINE__,
            errno, strerror(errno));
    exit(1);
  }
  

  /* Timer expiration will occur withing 3 seconds after being armed
   * by timer_settime().
   * the interval will be TIMER_INTERVAL seconds
   */
  trigger.it_value.tv_sec = EXPIRE_AFTER_ARM;
  if (is_timer_periodic) {
    trigger.it_interval.tv_sec = TIMER_INTERVAL;
  }

  /* Arm the timer. No flags are set
   * We we set the old ttrigger to NULL because we do not care about it
   * If you want to retrieve the old trigger, if any, set the last argument to
   * non-NULL
   */
  if (0 > timer_settime(timerid, /* timer ID: 3rd arg by timer_create() */
                        0 /* flags */,
                        &trigger /* new trigger */, 
                        NULL /* If non-NULL, return OLD trigger, if any */)) {
    fprintf(stderr, "%s %d: Cannot arm timer: %d %s\n",
            __FUNCTION__, __LINE__,
            errno, strerror(errno));
    exit(1);
  }

  /*
   * Let's print
   */
  fprintf(stdout, "%s %d my thread %ld going lock mutex then wait for %u, "
          "expiration after %lu seconds interval %lu seconds\n",
          __FUNCTION__, __LINE__, my_thread,
          MAIN_SLEEP_PERIOD,
          trigger.it_value.tv_sec, trigger.it_interval.tv_sec);

  pthread_mutex_lock(&one_thread_mutex);
  /* Wait seconds under the main thread. In 5 seconds (when the
   * timer expires), a message will be printed to the standard output
   * by the newly created notification thread.
   */
  sleep(MAIN_SLEEP_PERIOD);

  
  fprintf(stdout, "%s %d my thread %ld going UNLOCK mutex then wait for %u, "
          "expiration after %lu seconds interval %lu seconds \n",
          __FUNCTION__, __LINE__, my_thread,
          MAIN_SLEEP_PERIOD,
          trigger.it_value.tv_sec, trigger.it_interval.tv_sec);
  
  pthread_mutex_unlock(&one_thread_mutex);

  sleep(MAIN_SLEEP_PERIOD2);
  
  /* Delete (destroy) the timer 
   * IF the timer is NOT periodic, this means that the handler will actrually
   * re-arm it by calling timer_settime using the timerid. Hence do NOT delete
   * the timer using timer_delete() otherwise the call to timer_settime() will
   * crash     
   * */
  if (is_timer_periodic) {
    timer_delete(timerid);
  }
  
  return EXIT_SUCCESS;
}
 

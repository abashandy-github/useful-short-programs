/*
 *****************************************************************
 * How to build
 *-------------
 * Use
 *    gcc -o directory_lock_test  directory_lock_test.c 
 * 
 * How to use
 * -------------
 *   ./directory_lock_test  [directory] [number of seconds to sleep]
 * If you want to open a protected directory
 *   sudo ./directory_lock_test [directory] [number of seconds to sleep]
 *
 * example
 * --------
 *    sudo ./directory_lock_test  /dev/shm/ 8
 * Locks /dev/shm for 8 seconds
 * 
 * Example of testing advisory lock on a directory
 * ----------------------------------------------
 * - In one terminal, issue the following
 *     sudo ./directory_lock_test  /dev/shm/ 8
 *   You will see a print out saying
 *     Going to lock '/dev/shm/' fd=3  
 *     SUCCESSFULLY locked '/dev/shm/' fd=3 for 8 seconds
 * - VERY QUICKLY in another window do 
 *     sudo ./directory_lock_test /dev/shm/
 *   we should see that the program freezes after printing
 *     Going to lock '/dev/shm/' fd=3 for 0 seconds
 * - Then when command in first window returns, the second window exists
 *   after printing
 *     SUCCESSFully locked '/dev/shm/' fd=3
 *****************************************************************
 */
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <stdint.h>

#include <sys/file.h>        /* For "flock()" */
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <sys/types.h>

int
main (int		argc,
      char*		argv[])
{
  int fd;
  int sleep_period = 0;
  if (!argv[1]) {
    fprintf(stderr,"\nMUST specify directory to lock\n");
    exit (1);
  }
  if (argv[2]) {
    sleep_period = atoi(argv[2]);
  }
  fd = open (argv[1], O_DIRECTORY, S_IRWXO | S_IRWXG | S_IRWXU);
  if (fd < 0) {
      fprintf(stderr, "\nCannot open '%s' : %d %s\n", argv[1],
              errno, strerror(errno));
              
      return EXIT_FAILURE;
  }

  if (sleep_period) {
    fprintf(stdout, "Going to lock '%s' fd=%d\n",
            argv[1], fd);
  } else {
    fprintf(stdout, "Going to lock '%s' fd=%d for %d seconds\n",
            argv[1], fd, sleep_period);
  }
  if (flock(fd,LOCK_EX) < 0) {
    fprintf(stderr, "\nCannot LOCK '%s' fd=%d : %d %s\n", argv[1], fd,
            errno, strerror(errno));
    
    return EXIT_FAILURE;
  }
  if (!sleep_period) {
    fprintf(stdout, "SUCCESSFully locked '%s' fd=%d\n",
            argv[1], fd);
  } else {
    sleep(sleep_period);
    fprintf(stdout, "SUCCESSFULLY locked '%s' fd=%d for %d seconds\n",
            argv[1], fd, sleep_period);
    
  }
}
  

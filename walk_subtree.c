/*
 * I got this from "man nftw" on ubuntuu 24.04 in june/2/25
 * The main difference is that i had to also add
 *      #define __USE_XOPEN_EXTENDED 1
 * as explained next to the #define
 *
 * Also to understand the structure of the argment "sb",
 *   man -s3type stat
 *
 * To know how to decode gthe "st_mode" bitmask, do
 *   man -s7 inode
 *
 * For the time, I got the function 
 * For the permision I got some lines of code from
 https://codeforwin.org/c-programming/c-program-find-file-properties-using-stat-function
 *
 * To build (tested on Ubunti 24.04)
 *   gcc walk_subtree.c -o walk_subtree
 */
/* According to man nftw _XOPEN_SOURCE MUST be greater than 500
 * According to "man -s3type stat" _XOPEN_SOURCE MUST be greater than or equal
 * OR  _POSIX_C_SOURCE >= 200809L
 **/

#define _XOPEN_SOURCE 700
#define  _POSIX_C_SOURCE 200809L
#define __USE_XOPEN_EXTENDED 1 /* Needed by doing experiments on ubutnu 24.04
                                * for nftw(), otherwise I am not able to find
                                * ntfw() in the ftw.h header file and I will
                                * have to use ftw()
                                
                                * It was also one of the replies in
                                *   https://stackoverflow.com/questions/782338/warning-with-nftw*/

#include <ftw.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> /* needed for struct stat and the flags */
#include <time.h> /* needed for gmtime() */


/*
 * Got this funciton from the internet sometime in May/2025 and I mad some
 * changes 
 */
char* timespec_to_str(const struct timespec *ts, char *buf, size_t buf_size)
{
    time_t seconds = ts->tv_sec;
    long nanoseconds = ts->tv_nsec;
    struct tm tm_info;

    /* Convert seconds to struct tm (broken-down time) */
    if (localtime_r(&seconds, &tm_info) == NULL) {
        perror("localtime_r");
        return NULL;
    }
    
    /* Format the time string */
    if (strftime(buf, buf_size, "%Y-%m-%d %H:%M:%S", &tm_info) == 0) {
        fprintf(stderr, "strftime buffer too small\n");
        return NULL;
    }

    /* Append nanoseconds */
    int result = snprintf(buf + strlen(buf), buf_size - strlen(buf), ".%09ld",
                          nanoseconds);
    if (result < 0 || result >= buf_size - strlen(buf)){
        fprintf(stderr, "snprintf error or buffer too small\n");
        return NULL;
    }
    return buf;
}

static int
display_info(const char *fpath, const struct stat *sb,
             int tflag, struct FTW *ftwbuf)
{
  
  printf("%-3s %2d ",
         (tflag == FTW_D) ?   "d"   : (tflag == FTW_DNR) ? "dnr" :
         (tflag == FTW_DP) ?  "dp"  : (tflag == FTW_F) ?   "f" :
         (tflag == FTW_NS) ?  "ns"  : (tflag == FTW_SL) ?  "sl" :
         (tflag == FTW_SLN) ? "sln" : "???",
         ftwbuf->level);

  /* Print permission and file type bits the "st_mode" field
   *  see man -s7 inode for details
   *
   * NOTE: V.I. MUST use S_IFMT with the the file type bits
   * - The reason is as follows
   * - the type bits overlap
   * - Hence you MUST first mask the "st_mode" field with the "S_IFMT"
   *   then see of it is equal to the type
   */
    
  printf(" %s%s%s%s%s%s%s%s%s%s%s%s%s",
         /* File type.
         * Results SHOULD match the flags in "tflags"*/
         ((S_IFMT & sb->st_mode) == S_IFIFO ) ? "F" : /* 0010000 FIFO */
         ((S_IFMT & sb->st_mode) == S_IFDIR ) ? "d" : /* 0040000 directory */
         ((S_IFMT & sb->st_mode) == S_IFBLK ) ? "B" : /* 0060000 block device */
         ((S_IFMT & sb->st_mode) == S_IFREG ) ? "f" : /* 0100000 regular file */
         ((S_IFMT & sb->st_mode) == S_IFLNK ) ? "l" : /* 0120000 symbolic link*/
         ((S_IFMT & sb->st_mode) == S_IFSOCK) ? "S" : /* 0140000 socket */
         "?",

         /* Set bits */
         sb->st_mode & S_ISUID ? "U" : "-", /*set-user-ID bit (see execve(2))*/
         sb->st_mode & S_ISGID ? "G" : "-", /* set-group-ID bit */
         sb->st_mode & S_ISVTX ? "S" : "-", /* sticky bit */

         /* OWner permissions */
         sb->st_mode & S_IRUSR ? "r" : "-", 
         sb->st_mode & S_IWUSR ? "w" : "-", 
         sb->st_mode & S_IXUSR ? "x" : "-", 

         /* Group Permiossions */
         sb->st_mode & S_IRGRP ? "r" : "-", 
         sb->st_mode & S_IWGRP ? "w" : "-", 
         sb->st_mode & S_IXGRP ? "x" : "-", 

         /* Other Permiossions */
         sb->st_mode & S_IROTH ? "r" : "-", 
         sb->st_mode & S_IWOTH ? "w" : "-", 
         sb->st_mode & S_IXOTH ? "x" : "-");
  
  /* Print time
   * We will convert creation and modification time from seconds to date and
   * time format
   *
   * The fields are explained in "man -s3type stat"
   * ""man -s3type stat" says that "st_ctime" is change time
   * However based on expermiments, it is the time the file was created
   *
   * VERY IMPORTANT
   * - For backward compatibility, the "struct stat"  has the following
   *      #define st_atime  st_atim.tv_sec
   *      #define st_mtime  st_mtim.tv_sec
   *      #define st_ctime  st_ctim.tv_sec
   *   I.e. if you use something like "sb->st_atime", then you will actually
   *      get  "long int *" instead of "struct timespec *"
   * - The he fields of type "struct timespec" are WITHOUT the last "e"
   *   I.e. the field names that contain the second and nano seconds iare
   *        struct timespec  st_atim;
   *        struct timespec  st_mtim;
   *        struct timespec  st_ctim;

   * VERY IMPORTANT
   * When I tested this on Ubuntu 24.04 on Jun/9/25, I can see the following
   * - Sometimes, the "st_ctim" (inode change time) gives the SAME VALUE is
   *   "st_mtim" I.e. the inode change and file data modify are the same
   * - Sometimes I can see that the "st_ctim" (inode change time) is AFTER the
   *   "st_mtim" (file data modify time)
   * - Sometimes I see the "st_atim" (file data access time) and
   *   "st_mtim" (file data modify time) are IDENTICAL and they are both
   *   EARLiER tha "st_ctim" (inode change time)
   *
   */
  char timebuf[200];
   // File data modification time
   // For some reason, the "st_mtime" gives the SAME VALUE is "st_ctime"
  timespec_to_str(&sb->st_mtim, timebuf, sizeof(timebuf));
  printf(" Modified %s ", timebuf);
  
  // File data Access time
  timespec_to_str(&sb->st_atim, timebuf, sizeof(timebuf));
  printf(" Accessed %s ", timebuf);
   
  // Inode last change timestamp
  timespec_to_str(&sb->st_ctim, timebuf, sizeof(timebuf));
  printf(" inode Changed %s ", timebuf);
  
  /* Print the size */
  if (tflag == FTW_NS)
    printf("-------");
  else
     printf("%9jd", (intmax_t) sb->st_size);

  /*
   * - fpath: the entire path
   * - ftwbuf->base: the number of characters in the string "fpath" until yuou
   *   get to the last part of the path ONLY
   * - fpath + ftwbuf->base: Gives the pointer to ending part of the path
   */
  printf("   %-40s %d %s\n",
         fpath, ftwbuf->base, fpath + ftwbuf->base);
  
  return 0;           /* To tell nftw() to continue */
}

int
main(int argc, char *argv[])
{
  int flags = 0;
  char *dir_name = "./";
  int c;
  while ((c = getopt (argc, argv, "dps:")) != -1) {
    switch (c) {
    case 's':
      dir_name = optarg;
      break;
    case 'p':
      flags |= FTW_PHYS;
      break;
    case 'd':
      flags |= FTW_DEPTH;
    case 'h':
    default:
      if (c!= 'h') {
        printf("\nInvalid argument %c\n", c);
      }
      printf("\nUsage:\n");
      printf("%s [options]\n"
             "\t-d use FTW_DEPTH with nftw\n"
             "\t-p Use FTW_PHYS  with nftw\n"
             "\t-s <search directory> (default is current directory) \n ",
             argv[0]);
      exit (0);
    }
  }
  
  if (nftw((argc < 2) ? "." : dir_name, display_info, 20, flags)
      == -1)
    {
      perror("nftw");
      exit(EXIT_FAILURE);
    }
  
  exit(EXIT_SUCCESS);
}

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>        /* needed read(), write()*/
#include <stdio.h>         /* for printf() and fopen() and fclose()*/
#include <fcntl.h>         /* for open() */
#include <string.h> /* needed for strlen */

/*
 * NOTE: V.I.
 * Does NOT WORK correctly with /proc files (and probably other special files
 * I tried it with a /proc/<pid>/maps" and it copied only half of the lines
 */
int copy_file(char *filename_in, char *filename_out, size_t *copy_size_p)
{
  ssize_t file_in_size = 0, file_out_size = 0, read_size = 0, write_size = 0 ;
  int fd_in = -1, fd_out = -1;
  *copy_size_p = 0;
  fd_in = open(filename_in, O_RDONLY);
  if (fd_in < 0) {
    printf("%s %d: : Cannot open input file '%s': %d %s\n",
           __func__, __LINE__,
           filename_in,
           errno, strerror(errno));
    goto out;
  }
  fd_out = open(filename_out,
                O_CREAT  | /* Create the file if it does not exist */
                O_RDWR   | /* Open fil1e for reading and writing */
                O_TRUNC,   /* Clear the file if it already exists */
                S_IRWXU  | /* User has all permission */
                S_IRWXG  | /* Group has all permission */
                S_IRWXO    /* Othershas all permission */);
  if (fd_out < 0) {
    printf("%s  %d: : Cannot create/open output file '%s' : %d %s\n",
           __func__, __LINE__,
           filename_out,
           errno, strerror(errno));
    goto out;
  }
  /* Now copy the file */
  char buf[100]; /* You can pick any number. It is just memory */
  while ((read_size = read(fd_in, buf, 100)) > 0)  {
    if ((write_size = write(fd_out, buf, read_size)) != read_size) {
      if (write_size < 0) {
        printf("%s %d Cannot write %zd into output file '%s' fd=%d: %d %s.\n",
               __func__, __LINE__,
               read_size,
               filename_out, fd_out,
               errno, strerror(errno));
        goto out;
      } else {
        /*
         * It is possible, though unlikely, that when writing a buffer it occurs on multiple calls
         * In this case, AFAIK we should retry. But it is is too complex
         */
        printf("%s %d: Read %zd from '%s' fd=%d but wrote %zd into '%s' fd=%d: %d %s\n",
               __func__, __LINE__,
               read_size,
               filename_in, fd_in,
               write_size,
               filename_out, fd_out,
               errno, strerror(errno));
      }
      break;
    } /* write_size != read_size */
    file_in_size += read_size;
    file_out_size += write_size;
  } /* while ((read_size = read(fd_in, buf, 100)) > 0) */

  
  /*
   * we are out of the while loop, let's see why we are out
   * read() returns 0 or +ve when we finish (or close to finishing)
   * reading the file but returns -ve if there is an error
   */
  if (read_size < 0) {
    printf("%s %d Cannot read from file '%s' fd=%d "
           "after reading %zu and writing %zu: %d %s.\n",
           __func__, __LINE__,
           filename_in, fd_in,
           file_in_size, file_out_size,
           errno, strerror(errno));
  } else {
    if (file_in_size != file_out_size) {
      printf("%s %d: CANNOT fully copy '%s' to '%s'. "
             "Read %zd but wrote %zd: %d %s\n",
             __func__, __LINE__,
             filename_in, filename_out,
             file_in_size, file_out_size,
             errno, strerror(errno));
      goto out;
    }
  }

  /* Close the file descriptor even if they have not been opened
   * close() will not be very unhappy (just returns EBADF) if the file
   * descriptor is -ve
   */
 out:
  if (write_size > -1) {
    *copy_size_p = (size_t)write_size;
  }
  close(fd_in);
  close(fd_out);
  return (errno);
}


int main(int argc, char *argv[])
{
  int retval = EINVAL;
  size_t copy_size;
  if (argc != 3) {
    printf("Usuage\n");
    printf("%s <source_file_path> <destination_file_path>\n", argv[0]);
    goto out;
  }

  retval = copy_file(argv[1], argv[2], &copy_size);
  if (retval) {
    printf("Cannot copy %s to %s. Only copied %zu: %d %s\n",
          argv[1], argv[2], copy_size,
          errno, strerror(errno));
  } else {
    printf("Successfully copied %zu bytes from '%s' to '%s'\n",
           copy_size, argv[1], argv[2]);
  }
 out:
  return (retval);
}

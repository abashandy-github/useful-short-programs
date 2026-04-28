/*
 * This is a simple program that takes an unsigned uint64_t in decimal or hexa
 * Allows the user to shitf left then/or shift right then/or bit-wise AND
 * The order of operation is as follows:
 * - If the user specifies shift left, then the shitf left will occur first
 * - If the user specifies shift right, then the shitf right is done AFTER the
 *   shift left but before the bitwise-AND
 * - If the user specifies the bitwise-AND then the bitwise-AND will be done
 *   after ay specified shift-left and shitf-right
 * After applying the specified operations (if any), it formats and prints out
 * the number and positions of the 1 in its binary fomrat
 * How to build
 * - Without debugs
 *     gcc Wall -Wno-comment count_num_ones.c -o count_num_ones
 * - With debug info (e.g. to use with GDB)
 *     gcc -O0 -g -Wall -Wno-comment count_num_ones.c -o count_num_ones
 *
 * Example
 *  $ ./count_num_ones -l4 -r8 -a0xfffff  0xfa65287613
 *  The number 'fa65287613' which is also '0x1075438974483' 
 *  	left-shifted by 4 bits to give 0xfa652876130 then 
 *  	right-shifted by 8 bits to give 0xfa6528761 then 
 *  	bitwise ANDed with 0xfffff (1048575) to give 0x28761
 *  1's positions: 0 5 6 8 9 10 15 17
 *  has 8 1's
 * 
 * Example
 * $ ./count_num_ones -a0xff 0x000300030003c303
 * The number '300030003c303' which is also '0x844437815280387' 
 * 	bitwise ANDed with 0xff (255) to give 0x3
 * 1's positions: 0 1
 * has 2 1's
 *
 * example
 * $ ./count_num_ones -r4 0x000300030003c303
 * The number '300030003c303' which is also '0x844437815280387' 
 * 	right-shifted by 4 bits to give 0x300030003c30
 * 1's positions: 4 5 10 11 12 13 28 29 44 45
 * has 10 1's
 *
 * Example
 * $ ./count_num_ones -l8 0x000300030003c303
 * The number '300030003c303' which is also '0x844437815280387' 
 * 	left-shifted by 8 bits to give 0x300030003c30300
 * 1's positions: 8 9 16 17 22 23 24 25 40 41 56 57
 * has 12 1's
 * 
 * Example
 * $ ./count_num_ones 0x000300030003c303
 * The number '300030003c303' which is also '0x844437815280387' 
 * 1's positions: 0 1 8 9 14 15 16 17 32 33 48 49
 * has 12 1's
 *
 * Example 2
 * $ ./count_num_ones 0x300030003c303
 * The number '300030003c303' which is also '0x844437815280387' 
 * 1's positions: 0 1 8 9 14 15 16 17 32 33 48 49
 * has 12 1's
 *
 * Example
 * $ ./count_num_ones -c -a0xff 0x000300030003c303
 * The number '300030003c303' which is also '0x844437815280387' 
 * 	bitwise ANDed with 0xff (255) to give 0x3
 * has 2 1's
 */


#include <stdio.h> /* for printf */
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h> /* for atoi, atol, atoll strtoull*/
#include <unistd.h>  /* for getopt */
#include <stdbool.h> /* for type "bool", true, false */
#include <inttypes.h> /* Fort PRIx64 */

int main (int argc, char *argv[])
{
  bool list_positions = true;
  uint64_t num, original;
  uint32_t shift_left = 0;
  uint32_t shift_right = 0;
  uint64_t and = UINT64_MAX;    
  uint32_t count = 0;
  int c;
  
  while ((c = getopt (argc, argv, "chl:r:a:")) != -1) {
    switch (c) {
    case 'c':
      list_positions = false;;
      break;
    case 'r':
      shift_right = atoi(optarg);
      break;
    case 'l':
      shift_left = atoi(optarg);
      break;
    case 'a':
      if (!strncmp(optarg, "0x", 2)) {
        and = strtoul(optarg, NULL, 16);
      } else {
        and = atol(optarg);
      }
      break;
    case 'h':
    default:
      if (c!= 'h') {
        printf("\nInvalid argument %c\n", c);
      }
      printf("\nUsage:\n");
      printf("%s [options] <number up to 2^64 - 1> \n"
             "Counts and list the positions of the 1's in the binary representation of <number>\n"
             "If <number> starts with 0x, it assumes it is hexa, otherwise it is decimal\n"
             "The order of operation is as follows:\n"
             "\t shift left\n"
             "\t shift right\n"
             "\t bitwise AND\n"                   
             "Options:\n"
             "\t-c do NOT list the positions that are 1.\n\t   The Default is %s\n"
             "\t-l <shift_left> shift number left by <shift_left> bits BEFORE doing shift_right and BEFORE counting bits.\n\t   The default is %d\n"
             "\t-r <shift_right> shift number right by <shift_right> AFTER doing the shift left but BEFORE counting bits.\n\t   Default is %d\n"
             "\t-a <and> bitwise AND number with <and> AFTER doing the shift but BEFORE counting bits.\n\t   Default is 0x%"PRIx64"\n"
             "\t-h print this message\n",
             argv[0],
             list_positions ? "list 1's positions" : "do NOT list 1's positions",
             shift_left,
             shift_right,
             and);
      exit (0);
    }
  }  

  /* Get the nmber
     physical system) if they were passed to us*/
  if (optind < argc) {
    if (!strncmp(argv[optind], "0x", 2)) {
      num = strtoul(argv[optind], NULL, 16);
    } else {
      num = atol(argv[optind]);
    }
    original = num;
  } else {
    printf("Must pass an integer to count the number of ones\n");
    exit (1);
  }

  /* 
   * apply the shits and bitwise and
   * The order is
   * 1, shift left\n"
   * 2. shift right\n"
   * 3 bitwise AND\n"                   
   */
  num = (num << shift_left);
  uint64_t num_after_shift_left = num;
  num = (num >> shift_right);
  uint64_t num_after_shift_left_then_right = num;    
  num = (num & and);
  uint64_t num_after_shift_left_then_right_then_and = num;    

  int i;
  printf("The number '%"PRIx64"' which is also '0x%"PRIu64"' ", original, original);
  if (shift_left) {
    printf("\n\tleft-shifted by %d bits to give 0x%"PRIx64"", shift_left, num_after_shift_left);
  }
  if (shift_right) {
    printf("%sright-shifted by %d bits to give 0x%"PRIx64"", shift_left ? " then \n\t" : "\n\t", shift_right, num_after_shift_left_then_right);
  }
  if (and != UINT64_MAX) {
    printf("%sbitwise ANDed with 0x%"PRIx64" (%"PRIu64") to give 0x%"PRIx64"",
           (shift_left || shift_right) ? " then \n\t" : "\n\t", 
           and, and,
           num_after_shift_left_then_right_then_and);
  }

  if (list_positions) {
  printf("\n");
    printf("1's positions: ");
  }
  bool is_first_num_printed = false;
  for (i = 0; i < sizeof(num)*8; i++) {
    if (num & 1) {
      count++;
      if (list_positions) {
        if (is_first_num_printed) {
          printf(" ");
        }
        printf("%u", i);
        is_first_num_printed = true;
      }
    }
    num = (num >> 1);
  }
  printf("\n");
  printf("has %u 1's\n", count);
  exit(0);
}
    

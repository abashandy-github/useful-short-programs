/*
 * This is a simple program that takes an unsigned uint64_t in decimal or hexa
 * format and prints out the number and positions of the 1 in its binary fomrat
 * How to build
 * - Without debugs
 * gcc count_num_ones.c -o count_num_ones
 * - With debug info (e.g. to use with GDB)
 *  gcc -O0 -g count_num_ones.c -o count_num_ones
 *
 * Example
 * $ ./count_num_ones 0x000300030003c303
 *   The number '844437815280387' which is also '0x300030003c303' has 12 1's
 *
 * Example 2
 * $ ./count_num_ones -l 0x300030003c303
 * 1's positions: 0, 1, 8, 9, 14, 15, 16, 17, 32, 33, 48, 49
 * The number '844437815280387' which is also '0x300030003c303' has 12 1's
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
    bool list_positions = false;
    uint64_t num, original;
    uint32_t count = 0;
    int c;
    
    while ((c = getopt (argc, argv, "lh")) != -1) {
        switch (c) {
        case 'l':
            list_positions = true;;
            break;
        case 'h':
        default:
            if (c!= 'h') {
                printf("\nInvalid argument %c\n", c);
            }
            printf("\nUsage:\n");
            printf("%s [options] <number up to 2^64 - 1> \n"
                   "Counts (and list the positions of ) the 1's in the binary representation of <number>\n"
                   "If <number> starts with 0x, it assumes it is hexa, otherwise it is decimal\n"
                   "Options:\n"
                   "\t-l list the positions that are 1 Default is %s\n"
                   "\t-h to print this message\n",
                   argv[0],
                   list_positions ? "true" : "false");
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

    int i;
    if (list_positions) {
        printf("1's positions: ");
    }
    bool is_first_num_printed = false;
    for (i = 0; i < sizeof(num)*8; i++) {
        if (num & 1) {
            count++;
            if (list_positions) {
                if (is_first_num_printed) {
                    printf(", ");
                }
                printf("%u", i);
                is_first_num_printed = true;
            }
        }
        num = (num >> 1);
    }
    if (list_positions) {
        printf("\n");
    }
    printf("The number '%"PRIu64"' which is also '0x%"PRIx64"' has %u 1's\n", original, original, count);
    exit(0);
}
    

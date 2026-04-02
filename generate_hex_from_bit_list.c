/*
 * This is a simple program that a list of numbers from 0 to 63 and will
 * generate a unit64_t from this list and print out the hexa and decimal values
 * of the gnerated number
 * format and prints out the number and positions of the 1 in its binary fomrat
 * How to build
 * - Without debugs
 * gcc generate_hex_from_bit_list.c -o generate_hex_from_bit_list
 * - With debug info (e.g. to use with GDB)
 *  gcc -O0 -g generate_hex_from_bit_list.c -o generate_hex_from_bit_list
 *
 * Example
 * bash-3.2$ ./generate_hex_from_bit_list 1 3 5 10 13 6
 * The number '0x246a' (also binary 10010001101010b and decimal '9322') has 6 1's
 *
 * Example 2
 * bash-3.2$ ./generate_hex_from_bit_list 1 3 5 10 50 6
 * The number '0x400000000046a' (also binary 100000000000000000000000000000000000000010001101010b and decimal '1125899906843754') has 6 1's
 * bash-3.2$ 
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
    uint64_t num = 0;
    uint32_t bit_position, i;
    int c;
    char buf[64 + 1];
    memset(buf, 0, 64 + 1);

    if (argc < 2) {
      fprintf(stderr, "MUST supply at least one bit position\n");
      exit(1);
    }    
    if (argc > 65) {
      fprintf(stderr, "List of bit positions cannot exceed 64\n");
      exit(1);
    }
    for (i = 1; i < argc; i++) {
      bit_position = atoi(argv[i]);
      if (bit_position > 63) {
        fprintf(stderr, "bit position %u MUST be in [0,63]\n", bit_position);
        exit(1);
      }
      num = num | ((uint64_t)1 << bit_position);
    }

    /* Get the bimary format
     * Unfortunately, there is no somthing like %b of PRIb64 in printf()
     */
    for (i=0; i < 64; i++) {
      buf[i] = (num & ((uint64_t)1<< (63-i))) ? '1' : '0';
    }
    // Skip ant leading zeroz */
    char *buf2 = buf;
    for (i = 0; i < strlen(buf); i++) {
      if (buf[i] == '1') {
        buf2 = &buf[i];
        break;
      }
    }

    
    printf("The number '0x%"PRIx64"' (also binary %sb and decimal '%"PRIu64"') has %u 1's\n",
           num, buf2, num, argc - 1);
    exit(0);
}
    

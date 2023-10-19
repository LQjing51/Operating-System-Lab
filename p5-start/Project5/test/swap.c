#include <stdio.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <time.h>
#include <stdlib.h>

#define NUM_PAGE 30
#define PAGE_SIZE 4096
#define OFFSET 23333333333lu

int main(int argc, char* argv[])
{
    if (argc != 3) {
        sys_move_cursor(1, 1);
        printf("Please input print_location and start_address\n");
        return 0;
    }
    int print_location = atol(argv[1]);
    long start_addr = atol(argv[2]);
	int i;
	sys_move_cursor(1, print_location);
    for (i = 0; i < NUM_PAGE; i++) {
        int *t = (int *) (start_addr + OFFSET + PAGE_SIZE * i);
        *t = i * 2 + 1;
        //printf("write %d to %ld\n",2*i+1,t);
    }
    for (i = 0; i < NUM_PAGE; i++) {
        int *t = (int *) (start_addr + OFFSET + PAGE_SIZE * i);
        int v = *t;
        if (v != i * 2 + 1) {
            printf("Error! addr[%d] = %d\n", i, v);
            return 0;
        }
    }
	printf("Success!\n");
	return 0;
}

#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

#include <os.h>

static char buff[64];

int main(int argc, char **argv)
{
    int start_pos = 0;
    if (argc > 1) {
        start_pos = atol(argv[1]);
    }
    int i, j;
    int fd = sys_fopen("1.txt", O_RDWR);
    sys_move_cursor(1, 1);
    printf("start postion: %d\n", start_pos);
    if (fd == -1) {
        printf("fopen failed!!!\n");
        return 0;
    }

    // write 'hello world!' * 10
    sys_lseek(fd, start_pos, 0);
    for (i = 0; i < 10; i++)
    {
        sys_fwrite(fd, "hello world!\n", 13);
    }

    // read
    sys_lseek(fd, -130, 1);
    for (i = 0; i < 10; i++)
    {
        sys_fread(fd, buff, 13);
        for (j = 0; j < 13; j++)
        {
            printf("%c", buff[j]);
        }
    }

    sys_close(fd);
}
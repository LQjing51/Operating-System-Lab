#include <stdio.h>
#include <sys/syscall.h>
#include <mthread.h>

#define PARALLEL_LEVEL 2    //make sure that PARALLEL_LEVEL <= 100
#define MAXN 100000         //make sure that PARALLEL_LEVEL divides MAXN

#define BINSEM_KEY 7
#define SHM_KEY 7

mthread_spinlock_t lock;
long sum;

void thread_addition(int id) {
    int gap = MAXN / PARALLEL_LEVEL;
    int st = gap * id + 1;
    int ed = st + gap;

    while (st < ed) {
        mthread_spinlock_lock(&lock);
        sum += st;
        mthread_spinlock_unlock(&lock);
        st++;
    }

    sys_exit();
}

int main(int argc, char* argv[])
{
    int print_location = 1;
    if (argc == 2) {
        //child process for multi-process addition
        int id = atol(argv[1]);
        int gap = MAXN / PARALLEL_LEVEL;
        int st = gap * id + 1;
        int ed = st + gap;

        volatile long *addr = shmpageget(SHM_KEY);
        int binsemid;
        binsemget(&binsemid,BINSEM_KEY);
        while (st < ed) {
            binsemop(binsemid, BINSEM_OP_LOCK);
            *addr += st;
            binsemop(binsemid, BINSEM_OP_UNLOCK);
            st++;
        }

        shmpagedt(addr);
        return 0;
    }

    int i;
    /*** multi-process ***/
//*
    sys_move_cursor(1, print_location);
    printf("multi-process: start computing from 1 to %d\n", MAXN);
    int st = clock();
    int pid[PARALLEL_LEVEL];
    char id[PARALLEL_LEVEL][3];
    char *newargv[2];
    for (i = 0; i < PARALLEL_LEVEL; i++) {
        id[i][0] = i / 10 + '0';
        id[i][1] = i % 10 + '0';
        id[i][2] = 0;
    }
    newargv[0] = argv[0];

    volatile long *addr = shmpageget(SHM_KEY);
    *addr = 0;
    for (i = 0; i < PARALLEL_LEVEL; i++) {
        newargv[1] = id[i];
        pid[i] = sys_exec(argv[0], 2, newargv, ENTER_ZOMBIE_ON_EXIT);
    }
    for (i = 0; i < PARALLEL_LEVEL; i++)
        sys_waitpid(pid[i]);
    sum = *addr;
    shmpagedt(addr);
    int ed = clock();
    printf("result: %ld\n", sum);
    printf("time costed: %d ticks\n", ed - st);
//*/
    /*** multi-thread ***/
//*
    printf("multi-thread: start computing from 1 to %d\n", MAXN);
    int st_ = clock();
    mthread_spinlock_init(&lock);
    mthread_t thread_task[PARALLEL_LEVEL];
    sum = 0;
    for (i = 0; i < PARALLEL_LEVEL; i++)
        mthread_create(&thread_task[i], thread_addition, i);
    for (i = 0; i < PARALLEL_LEVEL; i++)
        mthread_join(thread_task[i]);
    int ed_ = clock();
    printf("result: %ld\n", sum);
    printf("time costed: %d ticks\n", ed_ - st_);

    int A = ed - st, B = ed_ - st_;
    printf("linear speedup: %d.", A / B);
    printf("%d\n", 10 * A / B - 10 * (A / B));
//*/
    return 0;
}
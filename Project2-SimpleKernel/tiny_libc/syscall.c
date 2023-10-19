#include <sys/syscall.h>
#include <stdint.h>
//#include <os/sched.h>//
extern void do_scheduler(void);//
extern void port_write(char *str);
extern void vt100_move_cursor(int x, int y);
void sys_sleep(uint32_t time)
{
    // TODO:
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    // TODO:
   //port_write(buff);
   invoke_syscall(SYSCALL_WRITE, (long)buff, IGNORE, IGNORE);
}

void sys_reflush()
{
    // TODO:
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    // TODO:
    //vt100_move_cursor(x, y);
    invoke_syscall(SYSCALL_CURSOR, x, y,IGNORE);
}

long sys_get_timebase()
{
    // TODO:
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE);
}

long sys_get_tick()
{
    // TODO:
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE);
}

void sys_yield()
{
    // TODO:
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE);
    //   or
    //do_scheduler();
    // ???
}
char sys_read()
{
    return invoke_syscall(SYSCALL_READ, IGNORE, IGNORE, IGNORE);
}

uint32_t sys_get_wall_time(uint32_t *time_elapsed){
    //*time_elapsed=invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE);
    return invoke_syscall(SYSCALL_GET_TIME, time_elapsed, IGNORE, IGNORE);
}
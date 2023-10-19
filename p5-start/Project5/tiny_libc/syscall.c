#include <sys/syscall.h>
#include <stdint.h>
#include <os.h>
extern void do_scheduler(void);//
extern void port_write(char *str);
extern void vt100_move_cursor(int x, int y);
void sys_sleep(uint32_t time)
{
    // TODO:
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE , IGNORE);
}

void sys_write(char *buff)
{
    // TODO:
   //port_write(buff);
   invoke_syscall(SYSCALL_WRITE, (long)buff, IGNORE, IGNORE, IGNORE);
}

void sys_reflush()
{
    // TODO:
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    // TODO:
    //vt100_move_cursor(x, y);
    invoke_syscall(SYSCALL_CURSOR, x, y,IGNORE, IGNORE);
}

long sys_get_timebase()
{
    // TODO:
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE, IGNORE);
}

long sys_get_tick()
{
    // TODO:
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_yield()
{
    // TODO:
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE, IGNORE);
    //   or
    //do_scheduler();
    // ???
}
char sys_read()
{
    return invoke_syscall(SYSCALL_READ, IGNORE, IGNORE, IGNORE, IGNORE);
}

uint32_t sys_get_wall_time(uint32_t *time_elapsed){
    //*time_elapsed=invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE);
    return invoke_syscall(SYSCALL_GET_TIME, time_elapsed, IGNORE, IGNORE, IGNORE);
}


void sys_process_show(void){
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_screen_clear(void){
    invoke_syscall(SYSCALL_SCREEN_CLEAR, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_back_cursor(void){
    invoke_syscall(SYSCALL_BACK_CURSOR, IGNORE, IGNORE, IGNORE, IGNORE);
}

pid_t sys_spawn(task_info_t *info, void* arg, spawn_mode_t mode){
    return invoke_syscall(SYSCALL_SPAWN, (uintptr_t)info, (uintptr_t)arg, mode, IGNORE);
}
pid_t sys_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode){
    return invoke_syscall(SYSCALL_EXEC, (uintptr_t)file_name, argc, argv, mode);
}
void sys_exit(void){
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_kill(pid_t pid){
    return invoke_syscall(SYSCALL_KILL, pid, IGNORE, IGNORE, IGNORE);
}

int sys_waitpid(pid_t pid){
    return invoke_syscall(SYSCALL_WAITPID, pid, IGNORE, IGNORE, IGNORE);
}


pid_t sys_getpid(){
    return invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_get_char(){
    return invoke_syscall(SYSCALL_GET_CHAR, IGNORE, IGNORE, IGNORE, IGNORE);
}
pid_t sys_taskset(task_info_t *info, int mask){
    return invoke_syscall(SYSCALL_TASKSET, (uintptr_t)info, (uintptr_t)mask,IGNORE, IGNORE);
}
pid_t sys_taskset_p(int pid, int mask){
    return invoke_syscall(SYSCALL_TASKSET_P, (uintptr_t)pid, (uintptr_t)mask, IGNORE, IGNORE);
}
int sys_fork()
{
    return invoke_syscall(SYSCALL_FORK, IGNORE, IGNORE, IGNORE, IGNORE);
}
void* shmpageget(int key){
    return invoke_syscall(SYSCALL_SHMPAGEGET, key, IGNORE, IGNORE, IGNORE);
}
void shmpagedt(void *addr){
    invoke_syscall(SYSCALL_SHMPAGEDT, addr, IGNORE, IGNORE, IGNORE);
}

long sys_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength){
    return invoke_syscall(SYSCALL_NET_RECV,addr,length,num_packet,frLength);
}
void sys_net_send(uintptr_t addr, size_t length){
    invoke_syscall(SYSCALL_NET_SEND,addr,length,IGNORE,IGNORE);
}
void sys_net_irq_mode(int mode){
    invoke_syscall(SYSCALL_NET_IRQ_MODE,mode,IGNORE,IGNORE,IGNORE);
}

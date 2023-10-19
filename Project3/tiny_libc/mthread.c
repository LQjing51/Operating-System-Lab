#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>


int mthread_mutex_init(void* handle)
{
    /* TODO: */
    int *id=(int*)handle;
    *id=invoke_syscall(SYSCALL_LOCK_INIT, IGNORE, IGNORE, IGNORE);//mutex_get
    //do_mutex_lock_init(handle);
    return 0;
}
int mthread_mutex_lock(void* handle) 
{
    /* TODO: */
    //mutex_lock(*(int*)handle);
    //do_mutex_lock_acquire(handle);
    invoke_syscall(SYSCALL_LOCK_ACQUIRE,*(int*)handle, IGNORE, IGNORE);//mutex_lock
    return 0;
}
int mthread_mutex_unlock(void* handle)
{
    /* TODO: */
    //mutex_unlock(*(int*)handle);
    //do_mutex_lock_release(handle);
    invoke_syscall(SYSCALL_LOCK_RELEASE,*(int*)handle, IGNORE, IGNORE);//mutex_unlock
    return 0;
}

int mthread_barrier_init(void* handle, unsigned count)
{
    // TODO:
    int *id=(int*)handle;
    *id=invoke_syscall(SYSCALL_BARRIER_INIT, count, IGNORE, IGNORE);//barrier_get
}
int mthread_barrier_wait(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_BARRIER_WAIT,*(int*)handle, IGNORE, IGNORE);//barrier_wait
    return 0;
}
int mthread_barrier_destroy(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_BARRIER_DESTROY,*(int*)handle, IGNORE, IGNORE);//barrier_destory
    return 0;
}

int mthread_semaphore_init(void* handle, int val)
{
    // TODO:
    int *id=(int*)handle;
    *id=invoke_syscall(SYSCALL_SEMAPHORE_INIT, val, IGNORE, IGNORE);//barrier_get
}
int mthread_semaphore_up(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_SEMAPHORE_UP,*(int*)handle, IGNORE, IGNORE);//barrier_wait
    return 0;
}

int mthread_semaphore_down(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_SEMAPHORE_DOWN,*(int*)handle, IGNORE, IGNORE);//barrier_destory
    return 0;
}

int mthread_semaphore_destroy(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_SEMAPHORE_DESTROY,*(int*)handle, IGNORE, IGNORE);//barrier_destory
    return 0;
}
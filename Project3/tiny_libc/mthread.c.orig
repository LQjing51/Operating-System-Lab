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

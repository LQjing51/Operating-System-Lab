#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>

int mthread_spinlock_init(mthread_spinlock_t * spinlock){
    spinlock->value = 0;
    return 1;
}

int mthread_spinlock_lock(mthread_spinlock_t * spinlock){
    while(atomic_exchange(&(spinlock->value),1));
    return 1;
}
int mthread_spinlock_unlock(mthread_spinlock_t * spinlock){
    spinlock->value = 0;
    return 1;
}
int mthread_mutex_init(void* handle)
{
    /* TODO: */
    int *id=(int*)handle;
    *id=invoke_syscall(SYSCALL_LOCK_INIT, IGNORE, IGNORE, IGNORE,IGNORE);//mutex_get
    //do_mutex_lock_init(handle);
    return 0;
}
int mthread_mutex_lock(void* handle) 
{
    /* TODO: */
    //mutex_lock(*(int*)handle);
    //do_mutex_lock_acquire(handle);
    invoke_syscall(SYSCALL_LOCK_ACQUIRE,*(int*)handle, IGNORE, IGNORE,IGNORE);//mutex_lock
    return 0;
}
int mthread_mutex_unlock(void* handle)
{
    /* TODO: */
    //mutex_unlock(*(int*)handle);
    //do_mutex_lock_release(handle);
    invoke_syscall(SYSCALL_LOCK_RELEASE,*(int*)handle, IGNORE, IGNORE,IGNORE);//mutex_unlock
    return 0;
}
int binsemget(void* handle, int key)
{
    /* TODO: */
    int *id=(int*)handle;
    *id=invoke_syscall(SYSCALL_BINSEM_GET, key, IGNORE, IGNORE,IGNORE);//mutex_get
    //do_mutex_lock_init(handle);
    return *id;
}
int binsemop(int id,int op) 
{
    /* TODO: */
    //mutex_lock(*(int*)handle);
    //do_mutex_lock_acquire(handle);
    if (op == 0){
        invoke_syscall(SYSCALL_BINSEM_LOCK,id, IGNORE, IGNORE,IGNORE);//mutex_lock
    }else if(op == 1){
        invoke_syscall(SYSCALL_BINSEM_UNLOCK,id, IGNORE, IGNORE,IGNORE);//mutex_unlock
    } 
    return 0;
}

int mthread_barrier_init(void* handle, unsigned count)
{
    // TODO:
    int *id=(int*)handle;
    *id=invoke_syscall(SYSCALL_BARRIER_INIT, count, IGNORE, IGNORE,IGNORE);//barrier_get
}
int mthread_barrier_wait(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_BARRIER_WAIT,*(int*)handle, IGNORE, IGNORE,IGNORE);//barrier_wait
    return 0;
}
int mthread_barrier_destroy(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_BARRIER_DESTROY,*(int*)handle, IGNORE, IGNORE,IGNORE);//barrier_destory
    return 0;
}

int mthread_semaphore_init(void* handle, int val)
{
    // TODO:
    int *id=(int*)handle;
    *id=invoke_syscall(SYSCALL_SEMAPHORE_INIT, val, IGNORE, IGNORE,IGNORE);//barrier_get
}
int mthread_semaphore_up(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_SEMAPHORE_UP,*(int*)handle, IGNORE, IGNORE,IGNORE);//barrier_wait
    return 0;
}

int mthread_semaphore_down(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_SEMAPHORE_DOWN,*(int*)handle, IGNORE, IGNORE,IGNORE);//barrier_destory
    return 0;
}

int mthread_semaphore_destroy(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_SEMAPHORE_DESTROY,*(int*)handle, IGNORE, IGNORE,IGNORE);//barrier_destory
    return 0;
}
typedef pid_t mthread_t;

int mthread_create(mthread_t *thread, void (*start_routine)(void*), void *arg){
    *thread = invoke_syscall(SYSCALL_MTHREAD_CREATE, start_routine, arg, IGNORE, IGNORE);
    return 0;
}
int mthread_join(mthread_t thread){
    invoke_syscall(SYSCALL_MTHREAD_JOIN, thread, IGNORE, IGNORE, IGNORE);
    return 0;
}
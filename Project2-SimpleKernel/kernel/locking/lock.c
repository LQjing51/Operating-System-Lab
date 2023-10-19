#include <os/lock.h>
#include <os/sched.h>
#include <atomic.h>
#include <stdio.h>
#include <assert.h>
mutex_lock_t mutex_arr[5];
int count;
int mutex_get(){
    count++;
    do_mutex_lock_init(count);
    return count;
}
int mutex_lock(int handle){

    do_mutex_lock_acquire(&(mutex_arr[handle]));
    return 0;
}
int mutex_unlock(int handle){
    do_mutex_lock_release(&(mutex_arr[handle]));
    return 0;
}

void do_mutex_lock_init(int index)
{
    /* TODO */
  assert_supervisor_mode() ;
  
    list_init(&(mutex_arr[index].block_queue));
    mutex_arr[index].status = UNLOCKED;
  
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    /* TODO */
    if(lock->status == LOCKED){
        do_block(current_running,&(lock->block_queue));
    }else{
        lock->status = LOCKED;
    }
    /*while(lock->status == LOCKED){
        do_block(current_running,&(lock->block_queue));
    }
    lock->status = LOCKED;*/
}

void do_mutex_lock_release(mutex_lock_t *lock)
{
    /* TODO */
    if((lock->block_queue).next == &(lock->block_queue)){
        lock->status = UNLOCKED;
    }else{
        do_unblock((lock->block_queue).next);
        lock->status = LOCKED;
    }
}

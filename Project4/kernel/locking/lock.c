#include <os/lock.h>
#include <os/sched.h>
#include <os/string.h>
#include <atomic.h>
#include <stdio.h>
#include <assert.h>
#include <mailbox.h>
#include <os/mm.h>
mutex_lock_t mutex_arr[5];
extern void release_queue(list_head *queue);
int count=0;
binsem_lock_t binsem_arr[5];
int binsem_count = 0;

int mutex_get(){
    count++;
    do_mutex_lock_init(count);
    return count;
}
int mutex_lock(int handle){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    current_running[cpu_id]->lock[handle]=1;
    do_mutex_lock_acquire(&(mutex_arr[handle]));
    return 0;
}
int mutex_unlock(int handle){
    //uint64_t cpu_id;
    //cpu_id = get_current_cpu_id();
    //current_running[cpu_id]->lock[handle]=0;
    do_mutex_lock_release(&(binsem_arr[handle]));
    return 0;
}


int do_binsem_get(int key){
    int i=0;
    for(;i<5;i++){
        if(binsem_arr[i].key == key){
            return i;
        }
    }
    binsem_count++;
    binsem_arr[binsem_count].key = key;
    list_init(&(binsem_arr[binsem_count].block_queue));
    binsem_arr[binsem_count].status = UNLOCKED;
    return binsem_count;
}
int do_binsem_lock(int id){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    current_running[cpu_id]->lock[id]=1;
    do_mutex_lock_acquire(&(binsem_arr[id]));
    return 0;
}
int do_binsem_unlock(int id){
    do_mutex_lock_release(&(binsem_arr[id]));
    return 0;
}
void do_mutex_lock_init(int index)
{
    /* TODO */
 // assert_supervisor_mode() ;
  
    list_init(&(mutex_arr[index].block_queue));
    mutex_arr[index].status = UNLOCKED;
  
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    /* TODO */
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    if(lock->status == LOCKED){
        do_block(current_running[cpu_id],&(lock->block_queue));
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

barrier_t barrier_arr[5];
int barrier_count=0;

int barrier_get(unsigned count){
    barrier_count++;
    list_init(&(barrier_arr[barrier_count].block_queue));
    barrier_arr[barrier_count].need = count;
    barrier_arr[barrier_count].wait_num = 0;
    return barrier_count;
}

int barrier_wait(int handle){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
   /* if(barrier_arr[handle].need == 0){
        return 0;//has been destroyed
    }*/
    (barrier_arr[handle].wait_num)++;
    if(barrier_arr[handle].wait_num == barrier_arr[handle].need){
        barrier_arr[handle].wait_num = 0;

        release_queue(&(barrier_arr[handle].block_queue));
    }else{
        do_block(current_running[cpu_id],&(barrier_arr[handle].block_queue));
    }
    return 0;
}
int barrier_destroy(int handle){
    barrier_arr[handle].wait_num=0;
    barrier_arr[handle].need = 0;
    release_queue(&(barrier_arr[handle].block_queue));
    return 0;
}

semaphore_t semaphore_arr[5];
int semaphore_count=0;

int semaphore_init(int val){
    semaphore_count++;
    list_init(&(semaphore_arr[semaphore_count].block_queue));
    semaphore_arr[semaphore_count].value = val;
    return semaphore_count;
}
int semaphore_up(int handle){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    if(semaphore_arr[handle].value > 0){
        semaphore_arr[handle].value--;
        return 1;
    }else if(semaphore_arr[handle].value == 0){
        do_block(current_running[cpu_id],&(semaphore_arr[handle].block_queue));
    }
    return 0;
}
int semaphore_down(int handle){
    pcb_t* pcb_node;
    list_node_t* list_node;
    list_node = (semaphore_arr[handle].block_queue).next;
    if(list_node != &(semaphore_arr[handle].block_queue)){
        pcb_node = (pcb_t *)((char*)(list_node)-3*sizeof(reg_t));
        do_unblock(list_node);
        return 0;
    }else{
        semaphore_arr[handle].value++;
    }
    return 0;
}
int semaphore_destroy(int handle){
    semaphore_arr[handle].value=0;
    release_queue(&(semaphore_arr[handle].block_queue));

    return 0;
}
#define MAX_MBOX 5
#define LINE_LENGTH (MAX_MBOX_LENGTH + 1)
mbox_t mbox_arr[MAX_MBOX];
int mbox_count=1;
int kernel_mbox_open(char *name){
    int i=0;
    for(i=0;i < mbox_count ; i++){
        if(!kstrcmp(name,mbox_arr[i].name)){
            break;
        }
    }
    if(i == mbox_count){
        mbox_arr[mbox_count].status = OPEN;
        int j=0;
        //mbox_arr[mbox_count].name = (char*)kmalloc(sizeof(char)*50);
       // mbox_arr[mbox_count].data = (char*)kmalloc(sizeof(char)*LINE_LENGTH);
       //mbox_arr[mbox_count].data = (char*)kmalloc(sizeof(char)*MAX_MBOX_LENGTH);
        while(*name){
            mbox_arr[mbox_count].name[j++]=*name;
            name++;
        }
        mbox_arr[mbox_count].name[j]='\0';
        mbox_arr[mbox_count].index=0;
        mbox_arr[mbox_count].front=0;
        mbox_arr[mbox_count].rear=0;
        list_init(&(mbox_arr[mbox_count].send_queue));
        list_init(&(mbox_arr[mbox_count].recv_queue));
        mbox_count++;
        return mbox_count-1;
    }else{
        mbox_arr[i].status = OPEN;
        return i;
    }
    return 0;
}
void kernel_mbox_close(int mailbox){
    mbox_arr[mailbox].status = CLOSE;
}
int kernel_mbox_send(int mailbox, void *msg, int msg_length){
    int blocked_count=0;
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    for(;;){
       /* if(msg_length <= ((LINE_LENGTH - (mbox_arr[mailbox].rear - mbox_arr[mailbox].front + 1)) % LINE_LENGTH)){
            int i;
            for(i=0;i<msg_length;i++){
                mbox_arr[mailbox].data[mbox_arr[mailbox].rear] = ((char*)msg)[i];
                mbox_arr[mailbox].rear = (mbox_arr[mailbox].rear + 1) % LINE_LENGTH;
            }*/
        if(msg_length <= (MAX_MBOX_LENGTH-mbox_arr[mailbox].index)){
            int i;
            for(i=0;i<msg_length;i++){
                mbox_arr[mailbox].data[mbox_arr[mailbox].index++] = ((char*)msg)[i];
            }
            pcb_t* pcb_node;
            list_node_t* list_node;
            list_node = (mbox_arr[mailbox].recv_queue).next;
            while(list_node != &(mbox_arr[mailbox].recv_queue)){
                pcb_node = (pcb_t *)((char*)(list_node)-3*sizeof(reg_t));
                do_unblock(list_node);
                list_node = (mbox_arr[mailbox].recv_queue).next;
            }
            return blocked_count;
        }else{
            do_block(current_running[cpu_id],&(mbox_arr[mailbox].send_queue));
            blocked_count++;
        }
    }
}
int kernel_mbox_recv(int mailbox, void *msg, int msg_length){
    int blocked_count=0;
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    for(;;){
        /*if(msg_length <= ((mbox_arr[mailbox].rear - mbox_arr[mailbox].front + LINE_LENGTH) % LINE_LENGTH)){
            int i,j;
            int offset;
            for(i = 0; i < msg_length; i++){
                ((char*)msg)[i] = mbox_arr[mailbox].data[mbox_arr[mailbox].front] ;
                mbox_arr[mailbox].front = (mbox_arr[mailbox].front + 1) % LINE_LENGTH;
            }*/
          //  mbox_arr[mailbox].front = (--mbox_arr[mailbox].front)%LINE_LENGTH;
        if((mbox_arr[mailbox].index - msg_length) >= 0){
            int i,j;
            int offset;
            for(i=0,j=0;i<msg_length;i++,j++){
                ((char*)msg)[i] = mbox_arr[mailbox].data[j] ;
            }
            int remind = mbox_arr[mailbox].index-j;
            for(offset=0; offset < remind ;offset++){
                mbox_arr[mailbox].data[offset] = mbox_arr[mailbox].data[j++];
            }
            mbox_arr[mailbox].index = remind;
            pcb_t* pcb_node;
            list_node_t* list_node;
            list_node = (mbox_arr[mailbox].send_queue).next;
            while(list_node != &(mbox_arr[mailbox].send_queue)){
                pcb_node = (pcb_t *)((char*)(list_node)-3*sizeof(reg_t));
                do_unblock(list_node);
                list_node = (mbox_arr[mailbox].send_queue).next;
            }
            return blocked_count;
        }else{
            do_block(current_running[cpu_id],&(mbox_arr[mailbox].recv_queue));
            blocked_count++;
        }
    }
}
int count_length(char *msg){
    int i=0;
    char *tmp = msg;
    while(*tmp){
        i++;
        tmp++;
    }
    return i;
}
extern void in_queue(list_head* queue, pcb_t* pcb);
extern void out_queue(pcb_t* pcb);
extern void do_scheduler(void);
int kernel_send_recv(int* index, void *send_msg, void *recv_msg){
    int me = index[0];
    int to = index[1];
    int send_len;
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    int i;
    int recv_succ = 0;
    int send_succ = 0;
    if(me != 0 && to !=0){
        while(1){
            //recv
            /*if(mbox_arr[me].rear != mbox_arr[me].front){
                for(i = 0; i < ((mbox_arr[me].rear - mbox_arr[me].front + LINE_LENGTH) % LINE_LENGTH); i++){
                    ((char*)recv_msg)[i] = mbox_arr[me].data[mbox_arr[me].front] ;
                    mbox_arr[me].front = (mbox_arr[me].front + 1) % LINE_LENGTH;
                }*/
            if(mbox_arr[me].index > 0){
                int i,j;
                int offset;
                for(i=0,j=0;i<mbox_arr[me].index;i++,j++){
                    ((char*)recv_msg)[i] = mbox_arr[me].data[j] ;
                }
                /*int remind = mbox_arr[mailbox].index-j;
                for(offset=0; offset < remind ;offset++){
                    mbox_arr[mailbox].data[offset] = mbox_arr[mailbox].data[j++];
                }*/
                mbox_arr[me].index = 0;
                pcb_t* pcb_node;
                list_node_t* list_node;
                list_node = (mbox_arr[me].send_queue).next;
                while(list_node != &(mbox_arr[me].send_queue)){
                    pcb_node = (pcb_t *)((char*)(list_node)-3*sizeof(reg_t));
                    do_unblock(list_node);
                    list_node = (mbox_arr[me].send_queue).next;
                }
                recv_succ = 1;
            }
            //send
            send_len = kstrlen(send_msg);
            /*if(send_len <= ((LINE_LENGTH - (mbox_arr[to].rear - mbox_arr[to].front + 1)) % LINE_LENGTH)){
                for(i=0;i<send_len;i++){
                    mbox_arr[to].data[mbox_arr[to].rear] = ((char*)send_msg)[i];
                    mbox_arr[to].rear = (mbox_arr[to].rear + 1) % LINE_LENGTH;
                }*/
            if(send_len <= (MAX_MBOX_LENGTH-mbox_arr[to].index)){
                int i;
                for(i=0;i<send_len;i++){
                    mbox_arr[to].data[mbox_arr[to].index++] = ((char*)send_msg)[i];
                }
                pcb_t* pcb_node;
                list_node_t* list_node;
                list_node = (mbox_arr[to].recv_queue).next;
                while(list_node != &(mbox_arr[to].recv_queue)){
                    pcb_node = (pcb_t *)((char*)(list_node)-3*sizeof(reg_t));
                    do_unblock(list_node);
                    list_node = (mbox_arr[to].recv_queue).next;
                }
                send_succ = 1;
            }

            if(recv_succ == 1 && send_succ == 0){
                return 2;//send failed
            }else if(recv_succ == 0 && send_succ == 1){
                return 1;//recv failed
            }else if(recv_succ == 1 && send_succ == 1){
                return 0;//all success
            }else{
                do_block(current_running[cpu_id],&(mbox_arr[me].recv_queue));
                /*out_queue(current_running[cpu_id]);
                current_running[cpu_id]->status = TASK_BLOCKED;
                in_queue(&(mbox_arr[me].recv_queue),current_running[cpu_id]);
                in_queue(&(mbox_arr[to].send_queue),current_running[cpu_id]);
                do_scheduler();*/
            }
        }

    }else if(me == 0 && to !=0){
        //send
        send_len = kstrlen(send_msg);
        while(1){
            /*if(send_len <= ((LINE_LENGTH - (mbox_arr[to].rear - mbox_arr[to].front + 1)) % LINE_LENGTH)){
                for(i=0;i<send_len;i++){
                    mbox_arr[to].data[mbox_arr[to].rear] = ((char*)send_msg)[i];
                    mbox_arr[to].rear = (mbox_arr[to].rear + 1) % LINE_LENGTH;
                }*/
            if(send_len <= (MAX_MBOX_LENGTH-mbox_arr[to].index)){
                int i;
                for(i=0;i<send_len;i++){
                    mbox_arr[to].data[mbox_arr[to].index++] = ((char*)send_msg)[i];
                }
                pcb_t* pcb_node;
                list_node_t* list_node;
                list_node = (mbox_arr[to].recv_queue).next;
                while(list_node != &(mbox_arr[to].recv_queue)){
                    pcb_node = (pcb_t *)((char*)(list_node)-3*sizeof(reg_t));
                    do_unblock(list_node);
                    list_node = (mbox_arr[to].recv_queue).next;
                }
                return 0;
            }else{
                return 2;
                //do_block(current_running[cpu_id],&(mbox_arr[to].send_queue));
            }
        }
        //return 0;
    }else if(me != 0 && to ==0){
        while(1){
            //recv
            /*if(mbox_arr[me].rear != mbox_arr[me].front){
                for(i = 0; i < ((mbox_arr[me].rear - mbox_arr[me].front + LINE_LENGTH) % LINE_LENGTH); i++){
                    ((char*)recv_msg)[i] = mbox_arr[me].data[mbox_arr[me].front] ;
                    mbox_arr[me].front = (mbox_arr[me].front + 1) % LINE_LENGTH;
                }*/
            if(mbox_arr[me].index > 0){
                int i,j;
                int offset;
                for(i=0,j=0;i<mbox_arr[me].index;i++,j++){
                    ((char*)recv_msg)[i] = mbox_arr[me].data[j] ;
                }
                mbox_arr[me].index = 0;
                pcb_t* pcb_node;
                list_node_t* list_node;
                list_node = (mbox_arr[me].send_queue).next;
                while(list_node != &(mbox_arr[me].send_queue)){
                    pcb_node = (pcb_t *)((char*)(list_node)-3*sizeof(reg_t));
                    do_unblock(list_node);
                    list_node = (mbox_arr[me].send_queue).next;
                }
                return 0;
            }else{
                return 1;
               // do_block(current_running[cpu_id],&(mbox_arr[me].recv_queue));
            }
        }
        //return 0;
    }
    return 0;
}
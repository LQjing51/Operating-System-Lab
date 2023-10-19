#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>

#include<type.h>
pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE;
pcb_t pid0_pcb = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack,
    .user_sp = (ptr_t)pid0_stack,
    .preempt_count = 0,
    .status = TASK_READY
};
#define LIST_HEAD_INIT(name) {&(name),&(name)}
#define LIST_HEAD(name) list_head name = LIST_HEAD_INIT(name)

/* current running task PCB */
pcb_t * volatile current_running;

/* global process id */
pid_t process_id = 1;

void time_check();
void create_timer(uint64_t sleep);

list_head ready_queue;
LIST_HEAD(sleep_queue);

void list_init(list_head* queue){

    queue->next = queue;
    queue->prev = queue;
}
void list_add(list_node_t *head,list_node_t *node)
{
 
    list_node_t *pre;
    list_node_t *nxt;
    pre=head->prev;
    nxt=head;
    nxt->prev = node;
    node->next = nxt;
    node->prev = pre;
    pre->next = node;
}

void list_out(list_node_t * node)
{
    list_node_t *pre;
    list_node_t *nxt;
    
    pre=node->prev;
    nxt=node->next;
    pre->next=nxt;
    nxt->prev=pre;
  
}

void in_queue(list_head* queue, pcb_t* pcb)
{
    list_add(queue,&(pcb->list));
}
void out_queue(pcb_t* pcb)
{
    list_out(&(pcb->list));
}

pcb_t* pcb_addr(list_node_t * node){
    return (pcb_t *)((char*)(node)-3*sizeof(reg_t));
}
pcb_t * max_score(){
    pcb_t *next;
    list_node_t* tmp_node;
    next = NULL;
    tmp_node=ready_queue.next;
    int max = 0;
    int tmp = 0;
    while(tmp_node!=&ready_queue){
        tmp = pcb_addr(tmp_node)->tmp_priority;
        if(tmp>max){
            max=tmp;
            next=pcb_addr(tmp_node);
        }
        tmp_node = tmp_node->next;
    } 
    if(next!=NULL) next->tmp_priority = next->priority;
    tmp_node = ready_queue.next;
    while(tmp_node != &ready_queue){
        if(tmp_node != &next->list){
            pcb_addr(tmp_node)->tmp_priority++;
        }
        tmp_node = tmp_node->next;
    }
    return next;
}

uint32_t get_time(uint32_t *time_elapsed){
    *time_elapsed = get_ticks();
    return get_time_base();
}
void do_scheduler(void)
{
   assert_supervisor_mode() ;
    // TODO schedule
    // time_check();
    pcb_t *tmp = current_running;
    pcb_t *next;

   /* if(tmp->pid!=0 && tmp->status == TASK_RUNNING){
        if((tmp->list).next != &ready_queue){
            next=pcb_addr((tmp->list).next);
        }else{
            next=pcb_addr(((tmp->list).next)->next);
        }
    }else{
        next=pcb_addr(ready_queue.next);
    }*/
    next = max_score();
    current_running->status = TASK_READY;
    next->status = TASK_RUNNING;
    // Modify the current_running pointer.
    current_running = next;
    process_id = current_running->pid;
    // restore the current_runnint's cursor_x and cursor_y
    vt100_move_cursor(current_running->cursor_x,
                      current_running->cursor_y);
    screen_cursor_x = current_running->cursor_x;
    screen_cursor_y = current_running->cursor_y;
    // TODO: switch_to current_running
    switch_to(tmp,next);
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: sleep(seconds)
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the current_running
    // 2. create a timer which calls `do_unblock` when timeout
    create_timer(sleep_time*time_base);
    do_block(current_running,&sleep_queue);
    // 3. reschedule because the current_running is blocked.

    
}
void create_timer(uint64_t sleep){
    
    current_running->wakeup = sleep + get_ticks();
}

void time_check(){
    pcb_t* pcb_node;
    list_node_t* list_node;
    list_node = sleep_queue.next;
    while(list_node != &sleep_queue){

        pcb_node = pcb_addr(list_node);
        if(get_ticks() >= pcb_node->wakeup){
            //printk("unblock sleep node\n\r");
            list_node = list_node->next;
            do_unblock(&(pcb_node->list));
        }else{
            list_node = list_node->next;
        }
    }
}
void do_block(pcb_t *pcb_node, list_head *queue)
{
    // TODO: block the pcb task into the block queue

    current_running->status = TASK_BLOCKED;
    out_queue(pcb_node);
    in_queue(queue,pcb_node);
    do_scheduler();

}

void do_unblock(list_node_t *node)
{
    // TODO: unblock the `pcb` from the block queue
    pcb_t* pcb_node;
    pcb_node = pcb_addr(node);

    pcb_node->status = TASK_READY;
    out_queue(pcb_node);
    in_queue(&ready_queue,pcb_node);
}
reg_t do_fork(int priority){
    assert(process_id < NUM_MAX_TASK );
    pcb[process_id].kernel_sp = allocPage(1);
    pcb[process_id].user_sp = allocPage(1);
    pcb[process_id].pid = process_id + 1; 
    pcb[process_id].type = USER_PROCESS;
    pcb[process_id].priority = priority;
    pcb[process_id].tmp_priority = priority;
    pcb[process_id].status = TASK_READY;
 
    //pcb_t new = &(pcb[process_id]);
    
    kmemcpy(pcb[process_id].kernel_sp - PAGE_SIZE , current_running->kernel_sp + sizeof(regs_context_t) + sizeof(switchto_context_t) - PAGE_SIZE , PAGE_SIZE);
    kmemcpy(pcb[process_id].user_sp - PAGE_SIZE , current_running->user_sp - PAGE_SIZE , PAGE_SIZE);    
    
    pcb[process_id].kernel_sp = pcb[process_id].kernel_sp - sizeof(regs_context_t) - sizeof(switchto_context_t);
    regs_context_t *pt_regs =
        (regs_context_t *)(pcb[process_id].kernel_sp + sizeof(switchto_context_t));
    pt_regs->regs[10] = 0;//a0
    pt_regs->regs[2] += pcb[process_id].user_sp - current_running->user_sp;//sp
    pt_regs->regs[4] = (reg_t)&(pcb[process_id]);//tp
    pt_regs->regs[8] += pcb[process_id].user_sp - current_running->user_sp;//fp/s0
    
    in_queue(&ready_queue,&(pcb[process_id]));
    return process_id++;
}



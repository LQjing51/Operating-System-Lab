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
const ptr_t pid0_stack_m = INIT_KERNEL_STACK_M + PAGE_SIZE;
pcb_t pid0_pcb_m = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_m,
    .user_sp = (ptr_t)pid0_stack_m,
    .preempt_count = 0,
    .status = TASK_READY
};

const ptr_t pid0_stack_s = INIT_KERNEL_STACK_S + PAGE_SIZE;
pcb_t pid0_pcb_s = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_s,
    .user_sp = (ptr_t)pid0_stack_s,
    .preempt_count = 0,
    .status = TASK_READY
};

#define LIST_HEAD_INIT(name) {&(name),&(name)}
#define LIST_HEAD(name) list_head name = LIST_HEAD_INIT(name)

/* current running task PCB */
pcb_t * volatile current_running[2];

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
/*pcb_t* pcb_addr_wait(list_node_t * node){
    return (pcb_t *)((char*)(node)-3*sizeof(reg_t)-sizeof(list_node_t));
}*/
/*pcb_t * max_score(){
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
}*/

uint32_t get_time(uint32_t *time_elapsed){
    *time_elapsed = get_ticks();
    return get_time_base();
}

pcb_t* search_queue(){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    pcb_t* pcb_node;
    list_node_t* list_node;
    list_node = ready_queue.next;
    if(list_node->next==&ready_queue){
        return &pid0_pcb_s;//only_shell
    }
    list_node = list_node->next;
    pcb_node = pcb_addr(list_node);//pcb after shell
    //mask=1 :only 0 core
    //mask=2 :only 1 core
    //mask=3 :0 and 1 core
    while(list_node != &ready_queue && ((pcb_node->status == TASK_RUNNING && pcb_node->cpu != cpu_id) || pcb_node->mask == 1 )){
            list_node = list_node->next;
            pcb_node = pcb_addr(list_node);
        }
    if(list_node == &ready_queue){
        return &pid0_pcb_s;
    }else{
        return pcb_node;
    }
    /*if(pcb_node->status == TASK_READY && pcb_node->mask != 1 ){
        return pcb_node;//pcb after shell is schedulable
    }else{
        list_node = list_node->next;
        pcb_node = pcb_addr(list_node);
        if((list_node != &ready_queue) && (pcb_node->status == TASK_READY)){
            return pcb_node;//head->shell->cpu1_running->chosen pcb
        }
        /*while(list_node == &ready_queue || pcb_node->status != TASK_READY || pcb_node->mask == 0){
            list_node = list_node->next;
            pcb_node = pcb_addr(list_node);
        }*/
   // }
    //return &pid0_pcb_s;
}
void do_scheduler(void)
{
   //assert_supervisor_mode() ;
    // TODO schedule
    // time_check();
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    pcb_t *tmp = current_running[cpu_id];
    pcb_t *next;

    pcb_t *pcb_node;
    list_node_t *list_node;
    //list_node = (tmp->list).next;
    //pcb_node = pcb_addr(list_node);
    if(tmp->pid!=0 && tmp->status == TASK_RUNNING){
        list_node = (tmp->list).next;
        pcb_node = pcb_addr(list_node);
        int find=1;
        if(cpu_id == 0){
            //mask=1 :only 0 core
            //mask=2 :only 1 core
            //mask=3 :0 and 1 core
            while(list_node == &ready_queue || (pcb_node->status == TASK_RUNNING && pcb_node->cpu != cpu_id) ||  pcb_node->mask == 2){
                list_node = list_node->next;
                pcb_node = pcb_addr(list_node);
                if(pcb_node == tmp){
                    find=0;
                    break;
                }
            }
            next = find ? pcb_node : &pid0_pcb_m;
        }else{
            while(list_node == &ready_queue || (pcb_node->status == TASK_RUNNING && pcb_node->cpu != cpu_id) || pcb_node->pid == 1 || pcb_node->mask == 1){
                list_node = list_node->next;
                pcb_node = pcb_addr(list_node);
                if(pcb_node == tmp){
                    find=0;
                    break;
                }
            }
            next = find ? pcb_node : &pid0_pcb_s;
        }
    }else{
        if(cpu_id==0){
            next=pcb_addr(ready_queue.next);//shell
        }else{
            next=search_queue();
        }
    }
   // next = max_score();
    current_running[cpu_id]->status = (current_running[cpu_id]->status==TASK_RUNNING) ? 
                                    TASK_READY : current_running[cpu_id]->status;
    if(current_running[cpu_id]->killed == 1){
        current_running[cpu_id]->killed = 0;
        current_running[cpu_id]->pid = 0;
        out_queue(current_running[cpu_id]);
        current_running[cpu_id]->status = TASK_EXITED;
        int j;
        for(j=0;j<5;j++){
            if((current_running[cpu_id]->lock[j])==1){
                mutex_unlock(j);
                current_running[cpu_id]->lock[j]=0;
            }
        }
        
        free(current_running[cpu_id]->user_stack_base);
        free(current_running[cpu_id]->kernel_stack_base);
    
        release_queue(&(current_running[cpu_id]->wait_list));
    }

    next->status = TASK_RUNNING;
    next->cpu = cpu_id;
   // if(next->pid != 1 && next_pid != 0) prints("cpu_id = %d\n",next->cpu);
    // Modify the current_running pointer.
    current_running[cpu_id] = next;
    //process_id = current_running->pid;
    // restore the current_runnint's cursor_x and cursor_y
   // vt100_move_cursor(current_running[cpu_id]->cursor_x,
     //                 current_running[cpu_id]->cursor_y);
    //screen_cursor_x = current_running->cursor_x;
    //screen_cursor_y = current_running->cursor_y;
    // TODO: switch_to current_running
    switch_to(tmp,next);
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: sleep(seconds)
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the current_running
    // 2. create a timer which calls `do_unblock` when timeout
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    create_timer(sleep_time*time_base);
    do_block(current_running[cpu_id],&sleep_queue);
    // 3. reschedule because the current_running is blocked.

    
}
void create_timer(uint64_t sleep){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    current_running[cpu_id]->wakeup = sleep + get_ticks();
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
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    // TODO: block the pcb task into the block queue
    if(current_running[cpu_id]->status == TASK_RUNNING){ 
        out_queue(pcb_node); //out ready_queue
    }
    current_running[cpu_id]->status = TASK_BLOCKED;
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
   /* assert(process_id < NUM_MAX_TASK );
    pcb[process_id].kernel_sp = allocPage(1);
    pcb[process_id].user_sp = allocPage(1);
    pcb[process_id].pid = process_id + 1; 
    pcb[process_id].type = USER_PROCESS;
   // pcb[process_id].priority = priority;
   // pcb[process_id].tmp_priority = priority;
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
    
    in_queue(&ready_queue,&(pcb[process_id]));*/
    return process_id++;
}

char * trans(task_status_t status){
    switch(status){
        case TASK_BLOCKED : return "TASK_BLOCKED";
        case TASK_RUNNING : return "TASK_RUNNING";
        case TASK_READY   : return "TASK_READY";
        case TASK_ZOMBIE  : return "TASK ZOMBIE";
        case TASK_EXITED  : return "TASK_EXITED";
        default : return "Unknown type";
    }
}
void do_process_show(){
    prints("[PROCESS TABLE]\n");
    int i=0;
    //prints("[0] PID : 0 STATUS : %s\n", trans(pid0_pcb.status));
    while(i < NUM_MAX_TASK){
       // printk("[0] PID : 0 STATUS : %s\n",pid0_pcb.status);
        if(pcb[i].pid !=0 ){
            prints("[%d] PID : %d STATUS : %s MASK : 0x%d", i+1, pcb[i].pid, trans(pcb[i].status),pcb[i].mask);
            if(pcb[i].status == 1){//running
                prints(" on core %d\n",pcb[i].cpu);
            }else{
                prints("\n");
            }
        
        }
        i++;
    }
}

ptr_t free_space[NUM_MAX_FREE];
//int free_index=0;
void free(ptr_t free_addr_base){
    int j;
    for(j=0;j<NUM_MAX_FREE;j++){
        if(free_space[j]==0){
            free_space[j]=free_addr_base;//high address:one page
            break;
        }
    }
}   
void release_queue(list_head *queue){
   // pcb_t* pcb_node;
    list_node_t* list_node;
    list_node = queue->next;
    while(list_node != queue){
       // pcb_node = pcb_addr(list_node);
        do_unblock(list_node);
        list_node = queue->next;
    }
}

void do_exit(void){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    int i;
    if(current_running[cpu_id]->mode == AUTO_CLEANUP_ON_EXIT){
        current_running[cpu_id]->status = TASK_EXITED;

        out_queue(current_running[cpu_id]);
        
        current_running[cpu_id]->pid = 0;
        free(current_running[cpu_id]->user_stack_base);
        free(current_running[cpu_id]->kernel_stack_base);
        //list_init(&(current_running[cpu_id]->wait_list));
        
        for(i=0;i<5;i++){
            if((current_running[cpu_id]->lock[i])==1){
                mutex_unlock(i);
                current_running[cpu_id]->lock[i]=0;
            }
        }
        release_queue(&(current_running[cpu_id]->wait_list));
  
        do_scheduler();
    }else if(current_running[cpu_id]->mode == ENTER_ZOMBIE_ON_EXIT){
        current_running[cpu_id]->status = TASK_ZOMBIE;
        out_queue(current_running[cpu_id]);
        //int i;
        for(i=0;i<5;i++){
            if((current_running[cpu_id]->lock[i])==1){
                mutex_unlock(i);
                current_running[cpu_id]->lock[i]=0;
            }
        }
        release_queue(&(current_running[cpu_id]->wait_list));
  
        do_scheduler();
    }
}

int do_kill(pid_t pid){
    int i;
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    int j;
    for(i=0 ; (pcb[i].pid != pid) && (i < NUM_MAX_TASK) ; i++) ;
    if(i==NUM_MAX_TASK) return 0;
    if(pcb[i].status == TASK_RUNNING ){
        //pcb[i].status = TASK_EXITED;
        pcb[i].killed=1;
    }else{
        pcb[i].status = TASK_EXITED;
        pcb[i].pid = 0;
        //list_init(&(pcb[i].wait_list));
        out_queue(&pcb[i]);
        int j;
        for(j=0;j<5;j++){
            if((pcb[i].lock[j])==1){
                mutex_unlock(j);
                pcb[i].lock[j]=0;
            }
        }
        free(pcb[i].user_stack_base);
        free(pcb[i].kernel_stack_base);
    
        release_queue(&(pcb[i].wait_list));
    }

    return 1;
}

int do_waitpid(pid_t pid){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    int i;
    for(i=0 ; (pcb[i].pid != pid) && (i < NUM_MAX_TASK) ; i++) ;
    if(i==NUM_MAX_TASK) return 0;
    if(pcb[i].status == TASK_ZOMBIE){
        pcb[i].pid = 0;
        free(pcb[i].user_stack_base);
        free(pcb[i].kernel_stack_base);
        return 0;
    }    
    do_block(current_running[cpu_id],&(pcb[i].wait_list));
    if(pcb[i].pid!=0){
        pcb[i].pid = 0;
        free(pcb[i].user_stack_base);
        free(pcb[i].kernel_stack_base);
    }
    return 1;
}

pid_t do_getpid(){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    return current_running[cpu_id]->pid;
}


#include <os/list.h>
#include <os/mm.h>
#include <pgtable.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>
#include <csr.h>
#include<type.h>
#include <os/string.h>

extern void __global_pointer$();
extern void ret_from_exception();
pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack_m = INIT_KERNEL_STACK_M + PAGE_SIZE;
pcb_t pid0_pcb_m = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_m,
    .user_sp = (ptr_t)pid0_stack_m,
    .satp = (unsigned long)(((unsigned long)SATP_MODE_SV39 << SATP_MODE_SHIFT) 
            | ((unsigned long) 0 << SATP_ASID_SHIFT) | (PGDIR_PA>>NORMAL_PAGE_SHIFT)),
    //.preempt_count = 0,
    .status = TASK_READY
};

const ptr_t pid0_stack_s = INIT_KERNEL_STACK_S + PAGE_SIZE;
pcb_t pid0_pcb_s = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_s,
    .user_sp = (ptr_t)pid0_stack_s,
    .satp = (unsigned long)(((unsigned long)SATP_MODE_SV39 << SATP_MODE_SHIFT) 
            | ((unsigned long) 0 << SATP_ASID_SHIFT) | (PGDIR_PA>>NORMAL_PAGE_SHIFT)),
    //.preempt_count = 0,
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
        
        //free(current_running[cpu_id]->user_stack_base);
        //free(current_running[cpu_id]->kernel_stack_base);
        if( current_running[cpu_id]->is_thread == 0)
            current_running[cpu_id]->need_clean = 1;

        release_queue(&(current_running[cpu_id]->wait_list));
    }
    //clean other pcbs' page_table if need

    for (int i= 0; i< NUM_MAX_TASK; i++){
        if(pcb[i].need_clean == 1 && (&pcb[i] != current_running[0]) && (&pcb[i] != current_running[1])){
            pcb[i].pid = 0;
            ptr_t pt_addr = pcb[i].satp;
            uintptr_t ppn = pt_addr & ((1lu << 44) - 1);
            
            if(!pcb[i].is_thread)
                freePage(ppn);
            pcb[i].need_clean = 0;
        }
    }
    next->status = TASK_RUNNING;
    next->cpu = cpu_id;
    // Modify the current_running pointer.
    current_running[cpu_id] = next;

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

reg_t do_fork(){

    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    int i;
    for(i=0; i< NUM_MAX_TASK ; i++){
        if(pcb[i].pid==0){
            break;
        }
    }
    assert(i < NUM_MAX_TASK);
    pcb[i] = *current_running[cpu_id];//
    uintptr_t child_pt = create_pt();
    share_pgtable(child_pt, PGDIR_PA);

    uintptr_t satp = current_running[cpu_id]->satp;
    uintptr_t father_pt = (satp & ((1lu << 44) - 1)) << NORMAL_PAGE_SHIFT;
    fork_mem(child_pt,father_pt);//copy pgtable
    uintptr_t kernel_sp = pa2kva(allocPage(1,0));//alloc_page_helper((uintptr_t)(KERNEL_STACK_ADDR),child_pt,0);
    pcb[i].kernel_sp = kernel_sp + PAGE_SIZE;
    pcb[i].status = TASK_READY;
    pcb[i].pid = i+1;
    pcb[i].satp = (unsigned long)(((unsigned long)SATP_MODE_SV39 << SATP_MODE_SHIFT) 
            | ((unsigned long) pcb[i].pid << SATP_ASID_SHIFT) | (child_pt>>NORMAL_PAGE_SHIFT));
    list_init(&(pcb[i].wait_list));
    memcpy(pcb[i].kernel_sp - PAGE_SIZE , current_running[cpu_id]->kernel_sp + sizeof(regs_context_t) + sizeof(switchto_context_t) - PAGE_SIZE , PAGE_SIZE);

    regs_context_t *pt_regs =
        (regs_context_t *)(pcb[i].kernel_sp - sizeof(regs_context_t));  
   
    pt_regs->regs[4] = (reg_t)&pcb[i];//tp
    pt_regs->regs[10] = 0;//a0

    pcb[i].kernel_sp = pcb[i].kernel_sp - sizeof(regs_context_t) - sizeof(switchto_context_t);
    switchto_context_t *st_regs = (switchto_context_t *)pcb[i].kernel_sp;

    st_regs->regs[0] = (reg_t)&ret_from_exception;//ra                                       
    st_regs->regs[1] = (reg_t)pcb[i].kernel_sp;//sp
    
    in_queue(&ready_queue, &(pcb[i]));
    return pcb[i].pid;
    
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
        
        for(i=0;i<5;i++){
            if((current_running[cpu_id]->lock[i])==1){
                mutex_unlock(i);
                current_running[cpu_id]->lock[i]=0;
            }
        }
        release_queue(&(current_running[cpu_id]->wait_list));

        if(current_running[cpu_id]->is_thread == 0){
            current_running[cpu_id]->need_clean = 1;
        }else{
            current_running[cpu_id]->pid = 0;
        }

        do_scheduler();
    }else if(current_running[cpu_id]->mode == ENTER_ZOMBIE_ON_EXIT){
        current_running[cpu_id]->status = TASK_ZOMBIE;
        out_queue(current_running[cpu_id]);
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
        pcb[i].killed=1;
        
    }else{
        pcb[i].status = TASK_EXITED;

        out_queue(&pcb[i]);
        int j;
        for(j=0;j<5;j++){
            if((pcb[i].lock[j])==1){
                mutex_unlock(j);
                pcb[i].lock[j]=0;
            }
        }
        if(pcb[i].is_thread == 0)
            pcb[i].need_clean = 1;
    
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
            pcb[i].need_clean = 1;
        return 0;
    }
    if (pcb[i].status == TASK_EXITED) return 0;
    do_block(current_running[cpu_id],&(pcb[i].wait_list));
    if(pcb[i].pid!=0){
        pcb[i].status == TASK_EXITED;
        pcb[i].need_clean = 1;
    }
    return 1;
}

pid_t do_getpid(){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    return current_running[cpu_id]->pid;
}

pid_t do_mthread_create(void (*start_routine)(void*), void *arg){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    int i;
    for(i=0; i< NUM_MAX_TASK ; i++){
        if(pcb[i].pid==0){
            break;
        }
    }
    assert(i < NUM_MAX_TASK);
    uintptr_t satp = current_running[cpu_id]->satp;
    uintptr_t pt_addr = (satp & ((1lu << 44) - 1)) << NORMAL_PAGE_SHIFT;
    uintptr_t user_sp = alloc_page_helper((uintptr_t)(USER_STACK_ADDR + thread_stack_index*0x10000-PAGE_SIZE),pt_addr,0);
    //thread_stack_index++;
    uintptr_t kernel_sp= pa2kva(allocPage(1,0));//alloc_page_helper((uintptr_t)(USER_STACK_ADDR + thread_stack_index*0x10000 + PAGE_SIZE),pt_addr,0);
    pcb[i].mask = current_running[cpu_id]->mask;
    pcb[i].kernel_sp = kernel_sp + PAGE_SIZE;
    pcb[i].user_sp = USER_STACK_ADDR + thread_stack_index*0x10000;
    thread_stack_index++;
    pcb[i].kernel_stack_base =  pcb[i].kernel_sp;
    pcb[i].user_stack_base = pcb[i].user_sp;
    pcb[i].pid = 1+i;
    pcb[i].need_clean = 0;
    pcb[i].type = USER_PROCESS;
    pcb[i].status = TASK_READY;
    pcb[i].mode = current_running[cpu_id] -> mode;
    pcb[i].satp = current_running[cpu_id] -> satp;
    pcb[i].is_thread = 1;
    list_init(&(pcb[i].wait_list));

    int j;
    regs_context_t *pt_regs =
        (regs_context_t *)(pcb[i].kernel_sp - sizeof(regs_context_t));
    regs_context_t *father_regs =
        (regs_context_t *)(current_running[cpu_id] -> kernel_sp + sizeof(switchto_context_t));
    reg_t father_gp = father_regs->regs[3];    
    pt_regs->regs[2] = (reg_t)pcb[i].user_sp;//sp
    pt_regs->regs[3] = father_gp;//gp
    pt_regs->regs[4] = (reg_t)&pcb[i];//tp
    pt_regs->regs[10]= (reg_t)arg;//a0
    pt_regs->sepc = start_routine;
    pt_regs->sstatus = SR_SUM;//enable kernel to read users PTE;

    pcb[i].kernel_sp = pcb[i].kernel_sp - sizeof(regs_context_t) - sizeof(switchto_context_t);
    switchto_context_t *st_regs = (switchto_context_t *)pcb[i].kernel_sp;

    st_regs->regs[0] = (reg_t)&ret_from_exception;//ra                                       
    st_regs->regs[1] = (reg_t)pcb[i].kernel_sp;//sp

    for(j = 2;j < 14;j++){
        st_regs->regs[j] = 0;    
    }  
    
    in_queue(&ready_queue, &(pcb[i]));
    return pcb[i].pid;
}
int do_mthread_join(pid_t thread){
   do_waitpid(thread);

}


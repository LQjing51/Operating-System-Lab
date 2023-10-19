/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <common.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/sched.h>
#include <screen.h>
#include <sbi.h>
#include <stdio.h>
#include <os/time.h>
#include <os/syscall.h>
#include <test.h>

#include <csr.h>
extern int mutex_get();
extern int mutex_lock(int handle);
extern int mutex_unlock(int handle);
extern int barrier_get(unsigned count);
extern int barrier_wait(int handle);
extern int barrier_destroy(int handle);
extern int semaphore_init(int val);
extern int semaphore_up(int handle);
extern int semaphore_down(int handle);
extern int semaphore_destroy(int handle);
extern int kernel_mbox_open(char *name);
extern void kernel_mbox_close(int mailbox);
extern int kernel_mbox_send(int mailbox, void *msg, int msg_length);
extern int kernel_mbox_recv(int mailbox, void *msg, int msg_length);
extern int kernel_send_recv(int* index, void *send_msg, void *recv_msg);
extern void ret_from_exception();
extern void __global_pointer$();

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,void*arg,
    pcb_t *pcb)
{
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    
    /* TODO: initialization registers
     * note: sp, gp, ra, sepc, sstatus
     * gp should be __global_pointer$
     * To run the task in user mode,
     * you should set corresponding bits of sstatus(SPP, SPIE, etc.).
     */
    int i;
    pt_regs->regs[2] = (reg_t)user_stack;//sp
    pt_regs->regs[3] = (reg_t)__global_pointer$;//gp
    pt_regs->regs[4] = (reg_t)pcb;//tp
    pt_regs->regs[10]= (reg_t)arg;
    pt_regs->sepc = entry_point;
    pt_regs->sstatus = 0;//0x00000020;
    pt_regs->scause = 0;
    pt_regs->sbadaddr = 0;
    // set sp to simulate return from switch_to
    /* TODO: you should prepare a stack, and push some values to
     * simulate a pcb context.
     */
    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    pcb->user_sp = user_stack;
    switchto_context_t *st_regs = (switchto_context_t *)pcb->kernel_sp;

    //st_regs->regs[0] = entry_point;//ra
    st_regs->regs[0] = (reg_t)&ret_from_exception;//ra                                       
    st_regs->regs[1] = (reg_t)kernel_stack;//sp
    for(i = 2;i < 14;i++){
        st_regs->regs[i] = 0;    
    }  
    
}

static void init_pcb()
{
     /* initialize all of your pcb and add them into ready_queue
     * TODO:
     */
    current_running[0] = &pid0_pcb_m;
    current_running[1] = &pid0_pcb_s;
    task_info_t* task = NULL;
    int i;
    list_init(&ready_queue);
    /*for(process_id = 0 ; process_id < num_fork_tasks + num_timer_tasks + num_lock2_tasks + num_sched2_tasks; process_id++){
        if(process_id < num_lock2_tasks){
            task = lock2_tasks[process_id];
        }else if(process_id < num_lock2_tasks + num_sched2_tasks){
            task = sched2_tasks[process_id - num_lock2_tasks];
        }else if(process_id < num_lock2_tasks + num_sched2_tasks + num_timer_tasks){
            task = timer_tasks[process_id - num_lock2_tasks - num_sched2_tasks];
        }else{
            task = fork_tasks[process_id - num_lock2_tasks - num_sched2_tasks - num_timer_tasks];
        }
        pcb[process_id].kernel_sp = allocPage(1);
        pcb[process_id].user_sp = allocPage(1);
        pcb[process_id].pid = 1+process_id;
        pcb[process_id].priority = task->priority;
        pcb[process_id].tmp_priority = task->priority;
        //printk("pid=%d,kernel_sp=%010lx,entry_point=%010lx\n\r",pcb[i].pid,pcb[i].kernel_sp,task->entry_point);
        pcb[process_id].type = task->type;
        pcb[process_id].status = TASK_READY;
        init_pcb_stack(pcb[process_id].kernel_sp, pcb[process_id].user_sp, task->entry_point, &pcb[process_id]);
        in_queue(&ready_queue, &(pcb[process_id]));
    }*/
    for(i=0;i<NUM_MAX_TASK;i++){
        pcb[i].pid = 0;
    }

   struct task_info task_test_shell = {
    (uintptr_t)&test_shell, USER_PROCESS};
    //process_id = 0;
    task = &task_test_shell;
    pcb[0].mask =3;
    pcb[0].kernel_sp = allocPage(1);
    pcb[0].user_sp = allocPage(1);
    pcb[0].kernel_stack_base =  pcb[0].kernel_sp;
    pcb[0].user_stack_base = pcb[0].user_sp;
    pcb[0].pid = 1;
    //pcb[process_id].priority = task->priority;
    //pcb[process_id].tmp_priority = task->priority;
    pcb[0].type = task->type;
    pcb[0].status = TASK_READY;
    init_pcb_stack(pcb[0].kernel_sp, pcb[0].user_sp, task->entry_point, NULL, &pcb[0]);
    in_queue(&ready_queue, &(pcb[0]));
   // process_id++;
    
    /* remember to initialize `current_running`
     * TODO:
     */


}
pid_t do_spawn(task_info_t *info, void* arg, spawn_mode_t mode){
    int i;
    for(i=0; i< NUM_MAX_TASK ; i++){
        if(pcb[i].pid==0){
            break;
        }
    }
    if(i==NUM_MAX_TASK){
        prints("too much process!\n");
        return 0;
    }
    //ptr_t free_space[20];
    //int free_index=0;
    //int mask = *((int*)arg);
    int j;
    int kernel_stack_top=0;
    int user_stack_top=0;
    for(j=0;j<NUM_MAX_FREE;j++){
        if(free_space[j]!=0){
            kernel_stack_top=free_space[j];
            free_space[j]=0;
            break;
        }
    }
    for(;j<NUM_MAX_FREE;j++){
        if(free_space[j]!=0){
            user_stack_top=free_space[j];
            free_space[j]=0;
            break;
        }
    }
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    pcb[i].mask = current_running[cpu_id]->mask;
    pcb[i].kernel_sp = (kernel_stack_top!=0) ? kernel_stack_top : allocPage(1);
    pcb[i].user_sp = (user_stack_top!=0) ? user_stack_top : allocPage(1);
    pcb[i].kernel_stack_base =  pcb[i].kernel_sp;
    pcb[i].user_stack_base = pcb[i].user_sp;
    pcb[i].pid = 1+i;
    pcb[i].type = info->type;
    pcb[i].status = TASK_READY;
    pcb[i].mode = mode;
    list_init(&(pcb[i].wait_list));
    init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, info->entry_point, arg, &pcb[i]);
    in_queue(&ready_queue, &(pcb[i]));
    return i+1;
}
pid_t do_taskset(task_info_t *info, int mask){
    int i;
    for(i=0; i< NUM_MAX_TASK ; i++){
        if(pcb[i].pid==0){
            break;
        }
    }
    if(i==NUM_MAX_TASK){
        prints("too much process!\n");
        return 0;
    }
    int j;
    int kernel_stack_top=0;
    int user_stack_top=0;
    for(j=0;j<NUM_MAX_FREE;j++){
        if(free_space[j]!=0){
            kernel_stack_top=free_space[j];
            free_space[j]=0;
            break;
        }
    }
    for(;j<NUM_MAX_FREE;j++){
        if(free_space[j]!=0){
            user_stack_top=free_space[j];
            free_space[j]=0;
            break;
        }
    }
    pcb[i].mask = mask;
    pcb[i].kernel_sp = (kernel_stack_top!=0) ? kernel_stack_top : allocPage(1);
    pcb[i].user_sp = (user_stack_top!=0) ? user_stack_top : allocPage(1);
    pcb[i].kernel_stack_base =  pcb[i].kernel_sp;
    pcb[i].user_stack_base = pcb[i].user_sp;
    pcb[i].pid = 1+i;
    pcb[i].type = info->type;
    pcb[i].status = TASK_READY;
    pcb[i].mode = AUTO_CLEANUP_ON_EXIT;
    list_init(&(pcb[i].wait_list));
    init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, info->entry_point, NULL, &pcb[i]);
    in_queue(&ready_queue, &(pcb[i]));
    return i+1;
}
pid_t do_taskset_p(int pid, int mask){
    int i;
    for(i=0 ; (pcb[i].pid != pid) && (i < NUM_MAX_TASK) ; i++) ;
    if(i == NUM_MAX_TASK){
        return 0;
    }
    pcb[i].mask = mask;
}
static void init_syscall(void)
{
    // initialize system call table.
    syscall[SYSCALL_FORK       ] = &do_fork;//1
    syscall[SYSCALL_SLEEP       ] = &do_sleep;//2
    syscall[SYSCALL_LOCK_INIT   ] = &mutex_get;//3
    syscall[SYSCALL_LOCK_ACQUIRE] = &mutex_lock;//4
    syscall[SYSCALL_LOCK_RELEASE] = &mutex_unlock;//5
    syscall[SYSCALL_GET_TIME    ] = &get_time;//6
    syscall[SYSCALL_YIELD       ] = &do_scheduler;//7
    syscall[SYSCALL_WRITE       ] = &screen_write;//20  
    syscall[SYSCALL_READ        ] = &sbi_console_getchar;//21
    syscall[SYSCALL_CURSOR      ] = &screen_move_cursor;//22 
    syscall[SYSCALL_REFLUSH     ] = &screen_reflush;//23
    syscall[SYSCALL_GET_TIMEBASE] = &get_time_base;//30
    syscall[SYSCALL_GET_TICK    ] = &get_ticks;//31

    syscall[SYSCALL_PS          ] = &do_process_show;
    syscall[SYSCALL_SCREEN_CLEAR] = &screen_clear;
    syscall[SYSCALL_BACK_CURSOR ] = &screen_back;
    syscall[SYSCALL_SPAWN       ] = &do_spawn;
    syscall[SYSCALL_EXIT        ] = &do_exit;
    syscall[SYSCALL_KILL        ] = &do_kill;
    syscall[SYSCALL_WAITPID     ] = &do_waitpid;
    syscall[SYSCALL_GETPID      ] = &do_getpid;
    syscall[SYSCALL_BARRIER_INIT] = &barrier_get;
    syscall[SYSCALL_BARRIER_WAIT] = &barrier_wait;
    syscall[SYSCALL_BARRIER_DESTROY]=&barrier_destroy;
    syscall[SYSCALL_SEMAPHORE_INIT ]=&semaphore_init;
    syscall[SYSCALL_SEMAPHORE_UP   ]=&semaphore_up;
    syscall[SYSCALL_SEMAPHORE_DOWN ]=&semaphore_down;
    syscall[SYSCALL_SEMAPHORE_DESTROY]=&semaphore_destroy;
    syscall[SYSCALL_MBOX_OPEN   ] = &kernel_mbox_open;
    syscall[SYSCALL_MBOX_CLOSE  ] = &kernel_mbox_close;
    syscall[SYSCALL_MBOX_SEND   ] = &kernel_mbox_send;
    syscall[SYSCALL_MBOX_RECV   ] = &kernel_mbox_recv;
    syscall[SYSCALL_SEND_RECV   ] = &kernel_send_recv;

    syscall[SYSCALL_TASKSET     ] = &do_taskset;
    syscall[SYSCALL_TASKSET_P    ] = &do_taskset_p;
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main()
{
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    // init Process Control Block (-_-!)
    if(cpu_id == 0){
        init_pcb();
        printk("> [INIT] PCB initialization succeeded.\n\r");

        // read CPU frequency
        time_base = sbi_read_fdt(TIMEBASE);

        // init interrupt (^_^)
        init_exception();
        printk("> [INIT] Interrupt processing initialization succeeded.\n\r");

        // init system call table (0_0)
        init_syscall();
        printk("> [INIT] System call initialized successfully.\n\r");

        // fdt_print(riscv_dtb);

        // init screen (QAQ)
        init_screen();
        printk("> [INIT] SCREEN initialization succeeded.\n\r");

        smp_init();
        lock_kernel();
        disable_interrupt();
        wakeup_other_hart();
        enable_interrupt();
    
    }else{
        lock_kernel();
        setup_exception();
    }
    printk("running cpu_id = %d\n\r",cpu_id);
    // TODO:
    // Setup timer interrupt and enable all interrupt
    sbi_set_timer(get_ticks() + time_base/300);
    unlock_kernel();
    while (1) {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler();
       //enable_interrupt();
        //__asm__ __volatile__("wfi\n\r":::);
       //do_scheduler();
    };
    return 0;
}

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
extern void ret_from_exception();
extern void __global_pointer$();

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
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
    current_running = &pid0_pcb;
    task_info_t* task = NULL;
    int i;
    list_init(&ready_queue);
    for(process_id = 0 ; process_id < num_fork_tasks + num_timer_tasks + num_lock2_tasks + num_sched2_tasks; process_id++){
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
    }
    
    
    /* remember to initialize `current_running`
     * TODO:
     */


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
    
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main()
{
    // init Process Control Block (-_-!)
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

    // TODO:
    // Setup timer interrupt and enable all interrupt
   sbi_set_timer(get_ticks() + time_base/1000);
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

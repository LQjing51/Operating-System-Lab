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
#include <os/elf.h>
#include <pgtable.h>
#include <os/sched.h>
#include <screen.h>
#include <sbi.h>
#include <stdio.h>
#include <os/time.h>
#include <os/syscall.h>
#include <csr.h>

#include <plic.h>
#include <emacps/xemacps_example.h>
#include <net.h>
#include <assert.h>
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
extern int do_binsem_get(int key);
extern int do_binsem_lock(int id);
extern int do_binsem_unlock(int id);
extern int do_mthread_create(void (*start_routine)(void*), void *arg);
extern int do_mthread_join(pid_t thread);

extern void __global_pointer$();

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,ptr_t user_stack_kva,int argc, char* argv[],
    pcb_t *pcb)
{
    int j=0;
    int len;
    char* argv_copy[10];
    for(; j<argc;j++){
            len = kstrlen(argv[j]) + 1;
            kmemcpy(user_stack_kva-len,argv[j],len);
            user_stack_kva -= len;
            user_stack -= len;
            argv_copy[j] = user_stack;
    }
    kmemcpy(user_stack_kva-8*argc,argv_copy,8*argc);
    user_stack -= 8*argc;

    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    
    /* TODO: initialization registers
     * note: sp, gp, ra, sepc, sstatus
     * gp should be __global_pointer$
     * To run the task in user mode,
     * you should set corresponding bits of sstatus(SPP, SPIE, etc.).
     */
    int i;
    pt_regs->regs[2] = (reg_t)user_stack - user_stack % 128;//sp ,stack address needs to align to 128
    pt_regs->regs[3] = (reg_t)__global_pointer$;//gp
    pt_regs->regs[4] = (reg_t)pcb;//tp
    pt_regs->regs[10]= (reg_t)argc;//a0
    pt_regs->regs[11]= (reg_t)user_stack;//the beginning of argv_copy in user_stack
    pt_regs->sepc = entry_point;
    pt_regs->sstatus = SR_SUM;//enable kernel to read users PTE;
    pt_regs->scause = 0;
    pt_regs->sbadaddr = 0;
    
    
    // set sp to simulate return from switch_to
    /* TODO: you should prepare a stack, and push some values to
     * simulate a pcb context.
     */
    pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    pcb->user_sp = user_stack - user_stack % 128;
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
    //task_info_t* task = NULL;
    int i;
    list_init(&ready_queue);
    for(i=0;i<NUM_MAX_TASK;i++){
        pcb[i].pid = 0;
    }

    unsigned char *binary;
    int length;
    get_elf_file("shell",&binary,&length);
    uintptr_t pt_addr = create_pt();
    share_pgtable(pt_addr, PGDIR_PA);
    uintptr_t user_sp = alloc_page_helper((uintptr_t)(USER_STACK_ADDR-PAGE_SIZE),pt_addr,0);
    uintptr_t kernel_sp = pa2kva(allocPage(1,0));//alloc_page_helper((uintptr_t)(KERNEL_STACK_ADDR/*-PAGE_SIZE*/),pt_addr,0);    
    uintptr_t entry;
    entry = load_elf(binary, (unsigned)length, pt_addr, alloc_page_helper);

    pcb[0].mask = 3;
    pcb[0].need_clean = 0;
    pcb[0].kernel_sp = kernel_sp + PAGE_SIZE;// enable init_pcb to access kernel stack
    pcb[0].user_sp = USER_STACK_ADDR;
    pcb[0].kernel_stack_base =  pcb[i].kernel_sp;
    pcb[0].user_stack_base = pcb[i].user_sp;
    pcb[0].pid = 1;
    pcb[0].type = USER_PROCESS;
    pcb[0].status = TASK_READY;
    pcb[0].mode = AUTO_CLEANUP_ON_EXIT;
    pcb[0].satp = (unsigned long)(((unsigned long)SATP_MODE_SV39 << SATP_MODE_SHIFT) 
            | ((unsigned long) pcb[0].pid << SATP_ASID_SHIFT) | (pt_addr>>NORMAL_PAGE_SHIFT));
    list_init(&(pcb[0].wait_list));
    init_pcb_stack(pcb[0].kernel_sp, pcb[0].user_sp,entry, NULL, NULL, NULL, pcb);
    in_queue(&ready_queue, pcb);
   
   /*struct task_info task_test_shell = {
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
   */


}

pid_t do_exec(const char * task_name,int argc, char* argv[],spawn_mode_t mode){
    unsigned char *binary;
    int length;
    get_elf_file(task_name,&binary,&length);
    uintptr_t pt_addr = create_pt();
    share_pgtable(pt_addr, PGDIR_PA);
    uintptr_t user_sp = alloc_page_helper((uintptr_t)(USER_STACK_ADDR-PAGE_SIZE),pt_addr,0);
    uintptr_t kernel_sp = pa2kva(allocPage(1,0));//alloc_page_helper((uintptr_t)KERNEL_STACK_ADDR,pt_addr,0);    
    uintptr_t entry;
    entry = load_elf(binary, (unsigned)length, pt_addr, alloc_page_helper);
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
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    pcb[i].mask = /*(!kstrcmp(task_name,"swap")) ? 1:*/current_running[cpu_id]->mask;
    pcb[i].kernel_sp = kernel_sp + PAGE_SIZE;
    pcb[i].user_sp = USER_STACK_ADDR;
    pcb[i].kernel_stack_base =  pcb[i].kernel_sp;
    pcb[i].user_stack_base = pcb[i].user_sp;
    pcb[i].pid = 1+i;
    pcb[i].need_clean = 0;
    pcb[i].type = USER_PROCESS;
    pcb[i].status = TASK_READY;
    pcb[i].mode = mode;
    pcb[i].satp = (unsigned long)(((unsigned long)SATP_MODE_SV39 << SATP_MODE_SHIFT) 
            | ((unsigned long) pcb[i].pid << SATP_ASID_SHIFT) | (pt_addr>>NORMAL_PAGE_SHIFT));
    list_init(&(pcb[i].wait_list));
    init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, entry, user_sp+PAGE_SIZE, argc, argv,&pcb[i]);
    in_queue(&ready_queue, &(pcb[i]));
    return i+1;
                                
}
pid_t do_spawn(task_info_t *info, void* arg, spawn_mode_t mode){
    /*int i;
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
 
    net_poll_mode = 1;
    // xemacps_example_main();
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
    return i+1;*/
}
pid_t do_taskset(task_info_t *info, int mask){
    /*int i;
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
    return i+1;*/
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
    syscall[SYSCALL_TASKSET_P   ] = &do_taskset_p;
    syscall[SYSCALL_EXEC        ] = &do_exec;

    syscall[SYSCALL_BINSEM_GET  ] = &do_binsem_get;
    syscall[SYSCALL_BINSEM_LOCK ] = &do_binsem_lock;
    syscall[SYSCALL_BINSEM_UNLOCK]= &do_binsem_unlock;
    syscall[SYSCALL_MTHREAD_CREATE]=&do_mthread_create;
    syscall[SYSCALL_MTHREAD_JOIN] = &do_mthread_join;
    
    syscall[SYSCALL_SHMPAGEGET  ] = &shm_page_get;
    syscall[SYSCALL_SHMPAGEDT   ] = &shm_page_dt;

    syscall[SYSCALL_NET_RECV    ] = &do_net_recv;
    syscall[SYSCALL_NET_SEND    ] = &do_net_send;
    syscall[SYSCALL_NET_IRQ_MODE] = &do_net_irq_mode;
}

void init_net()
{
   
    uint32_t slcr_bade_addr = 0, ethernet_addr = 0;

    // get_prop_u32(_dtb, "/soc/slcr/reg", &slcr_bade_addr);
    slcr_bade_addr = sbi_read_fdt(SLCR_BADE_ADDR);
    printk("[slcr] phy: 0x%x\n\r", slcr_bade_addr);

    // get_prop_u32(_dtb, "/soc/ethernet/reg", &ethernet_addr);
    ethernet_addr = sbi_read_fdt(ETHERNET_ADDR);
    printk("[ethernet] phy: 0x%x\n\r", ethernet_addr);

    uint32_t plic_addr = 0;
    // get_prop_u32(_dtb, "/soc/interrupt-controller/reg", &plic_addr);
    plic_addr = sbi_read_fdt(PLIC_ADDR);
    printk("[plic] plic: 0x%x\n\r", plic_addr);

    uint32_t nr_irqs = sbi_read_fdt(NR_IRQS);
    // get_prop_u32(_dtb, "/soc/interrupt-controller/riscv,ndev", &nr_irqs);
    printk("[plic] nr_irqs: 0x%x\n\r", nr_irqs);

    XPS_SYS_CTRL_BASEADDR =
        (uintptr_t)ioremap((uint64_t)slcr_bade_addr, NORMAL_PAGE_SIZE);
    xemacps_config.BaseAddress =
        (uintptr_t)ioremap((uint64_t)ethernet_addr, NORMAL_PAGE_SIZE);
    uintptr_t _plic_addr =
        (uintptr_t)ioremap((uint64_t)plic_addr, 0x4000*NORMAL_PAGE_SIZE);
    
    //XPS_SYS_CTRL_BASEADDR = slcr_bade_addr;
    //xemacps_config.BaseAddress = ethernet_addr;
    xemacps_config.DeviceId        = 0;
    xemacps_config.IsCacheCoherent = 0;

    printk(
        "[slcr_bade_addr] phy:%x virt:%lx\n\r", slcr_bade_addr,
        XPS_SYS_CTRL_BASEADDR);
    printk(
        "[ethernet_addr] phy:%x virt:%lx\n\r", ethernet_addr,
        xemacps_config.BaseAddress);
    printk("[plic_addr] phy:%x virt:%lx\n\r", plic_addr, _plic_addr);
    plic_init(_plic_addr, nr_irqs);
    
    long status = EmacPsInit(&EmacPsInstance);
    if (status != XST_SUCCESS) {
        printk("Error: initialize ethernet driver failed!\n\r");
        assert(0);
    }
}
// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main()
{
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    // init Process Control Block (-_-!)
    clean_direct_pg();
    if(cpu_id == 0){
        init_net();
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
        
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler();
       //enable_interrupt();
        //__asm__ __volatile__("wfi\n\r":::);
       //do_scheduler();
    };
    return 0;
}

#include <os/irq.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/stdio.h>
#include <assert.h>
#include <sbi.h>
#include <screen.h>
#include <csr.h>
#include <pgtable.h>

#include <net.h>
#include <os/mm.h>
#include <emacps/xemacps_example.h>
#include <plic.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
uintptr_t riscv_dtb;

void reset_irq_timer()
{
    // TODO clock interrupt handler.
    // TODO: call following functions when task4
    screen_reflush();
    time_check();
    // net_check();
    // note: use sbi_set_timer
    sbi_set_timer(get_ticks()+time_base/300);
    // remember to reschedule
    do_scheduler();
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    // TODO interrupt handler.
    // call corresponding handler by the value of `cause`
    //printk("in helper:kernel_sp=%08lx\tcause=%08lx\n\r",regs,cause);
    if(cause & ((uint64_t) 1 << 63)){//the highest bit is 1:interrupt
       // printk("in irq\n\r");
        cause &= ~SCAUSE_IRQ_FLAG; 
        irq_table[cause](regs, stval, cause);
    }else{
       // printk("in exc\n\r");
        exc_table[cause](regs, stval, cause);
    }
    
}

void handle_int(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    reset_irq_timer();
}
void handle_inst_pagefault(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    printk("inst pagefault happened !");

}
void handle_pagefault(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    pcb_t *now_pcb = current_running[cpu_id];
    ptr_t pt_addr = now_pcb -> satp;
    uintptr_t ppn = pt_addr & ((1lu << 44) - 1);
    alloc_page_helper(stval,ppn<<NORMAL_PAGE_SHIFT,1);

}
extern uint64_t read_sip();
void handle_irq(regs_context_t *regs, int irq)
{
    // TODO: 
    // handle external irq from network device
    // let PLIC know that handle_irq has been finished
}

void init_exception()
{
    /* TODO: initialize irq_table and exc_table */
    /* note: handle_int, handle_syscall, handle_other, etc.*/
    int i;

    for(i = 0;i < IRQC_COUNT;i++){
        irq_table[i] = &handle_other;
    }
    irq_table[IRQC_S_TIMER] = &handle_int;

    for(i = 0;i < EXCC_COUNT;i++){
        exc_table[i] = &handle_other;
    }
    exc_table[EXCC_SYSCALL] = &handle_syscall;
    // exc_table[EXCC_INST_PAGE_FAULT ] = &handle_inst_pagefault;
    // exc_table[EXCC_LOAD_PAGE_FAULT ] = &handle_pagefault;
    // exc_table[EXCC_STORE_PAGE_FAULT] = &handle_pagefault;
    setup_exception();
}


void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    printk("hhhhhhhh something went wrong!");
    char* reg_name[] = {
        "zero "," ra  "," sp  "," gp  "," tp  ",
        " t0  "," t1  "," t2  ","s0/fp"," s1  ",
        " a0  "," a1  "," a2  "," a3  "," a4  ",
        " a5  "," a6  "," a7  "," s2  "," s3  ",
        " s4  "," s5  "," s6  "," s7  "," s8  ",
        " s9  "," s10 "," s11 "," t3  "," t4  ",
        " t5  "," t6  "
    };
    for (int i = 0; i < 32; i += 3) {
        for (int j = 0; j < 3 && i + j < 32; ++j) {
            printk("%s : %016lx ",reg_name[i+j], regs->regs[i+j]);
        }
        printk("\n\r");
    }
    printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    printk("stval: 0x%lx cause: %lx\n\r",
           stval, cause);
    printk("sepc: 0x%lx\n\r", regs->sepc);
    // printk("mhartid: 0x%lx\n\r", get_current_cpu_id());

    // while (1);
    /*
    uintptr_t fp = regs->regs[8], sp = regs->regs[2];
    
    printk("[Backtrace]\n\r");
    printk("  addr: %lx sp: %lx fp: %lx\n\r", regs->regs[1] - 4, sp, fp);
    // while (fp < USER_STACK_ADDR && fp > USER_STACK_ADDR - PAGE_SIZE) {
    while (fp > 0x10000) {
        uintptr_t prev_ra = *(uintptr_t*)(fp-8);
        uintptr_t prev_fp = *(uintptr_t*)(fp-16);

        printk("  addr: %lx sp: %lx fp: %lx\n\r", prev_ra - 4, fp, prev_fp);

        fp = prev_fp;
    }
    */

    assert(0);
}

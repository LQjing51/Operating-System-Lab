#include <os/syscall.h>

long (*syscall[NUM_SYSCALLS])();

void handle_syscall(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    // syscall[fn](arg1, arg2, arg3)
    regs->sepc = regs->sepc + 4;//regs[17]=a7
    regs->regs[10] = syscall[regs->regs[17]](regs->regs[10],//a0
                                              regs->regs[11],//a1
                                              regs->regs[12]);//a2
}

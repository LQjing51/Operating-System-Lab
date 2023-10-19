#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)
  /* TODO: 
   * save exception_handler_entry into STVEC
   * enable global exceptions */
  la t0, exception_handler_entry
  li t1, SR_SIE
  li t2, SIE_SSIE | SIE_STIE | SIE_SEIE
  csrw stvec, t0
  csrw sstatus, t1//interrupt_enable:sstaus_sie=1
  csrw sie, t2 //Supervisor_software_interrupt、s_timer_interrupt、s_syscall
  jr ra
ENDPROC(setup_exception)

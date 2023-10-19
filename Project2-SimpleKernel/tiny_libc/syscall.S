#include <asm.h>

ENTRY(invoke_syscall)
  /* TODO: */
  mv a7, a0 
  mv a0, a1
  mv a1, a2
  mv a2, a3
  ecall
  jr ra
ENDPROC(invoke_syscall)

ENTRY(sys_fork)
  li a7, 1//syscall_fork
  li a1, 0
  li a2, 0
  ecall
  jr ra
ENDPROC(sys_fork)

/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#define CSR_SIE			0x104
#define CSR_SIP			0x144
#define KERNEL_STACK		0x50500000

.section ".entry_function","ax"
.global _start
_start:
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop
  
  /* Clear BSS for flat non-ELF images */
    lui  s0,	  %hi(__bss_start)
    addi s0,s0, %lo(__bss_start)
    lui  s1,	  %hi(_end)
    addi s1,s1, %lo(_end)
    li   s3,    0
  bss_loop:
    sw   s3,    0(s0)
    addi s0,s0, 1
    ble  s0,s1, bss_loop

  /* setup C environment (set sp register)*/
    lui  sp,	%hi(KERNEL_STACK)
    addi sp,sp, %lo(KERNEL_STACK)

  /* Jump to the code in kernel.c*/
    j main

loop:
  wfi
  j loop


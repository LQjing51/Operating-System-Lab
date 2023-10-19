#include <asm/sbidef.h>
#include <asm/sbiasm.h>
#include <csr.h>

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50201000

// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50201000

// 3. os size location (get the size of your OS here)
.equ os_size_loc, 0x502001fa

.text
.global main

main:
	// fence on all memory and I/O
	fence

	// a0 is mhartid, a1 is the pointer of fdt
	move s0,a0
	move s1,a1

	// output a \n
	li a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR

	// The core whose mhartid=0 is selected to boot the kernel.
	// Other cores should wait until the kernel's initialization
	// has been finished.
 	move a0, s0
	bnez a0, secondary

	// your code
	lui a1, %hi(msg)
	addi a0, a1, %lo(msg) 
	SBI_CALL SBI_CONSOLE_PUTSTR
	
	lui  s0,	%hi(kernel)
	addi s0,s0, %lo(kernel)
	lui  t0,	%hi(os_size_loc)
	addi t0,t0, %lo(os_size_loc)
	li   s1,    64
	li   s2,    1
	slli s5, s1, 9//512*64

	li	 s3,	0
	lh   s4,    0(t0)
  	ble s4, s1, finish_read
read:
	move a0,  s0
	move a1,  s1
	move a2,  s2
	SBI_CALL SBI_SD_READ
	add  s0,  s0, s5
  	sub  s4,  s4, s1
	addi s2,  s2, 64
  	bgt  s4,  s1, read
finish_read:
	move a0,  s0
	move a1,  s4
	move a2,  s2
	SBI_CALL SBI_SD_READ

	lui  a0,	%hi(kernel_main)
	addi a0,a0, %lo(kernel_main)
	jr	 a0

secondary:
	/* TODO: 
	 * 1. Mask all interrupts
	 * 2. let stvec pointer to kernel_main
	 * 3. enable software interrupt for ipi
	 */
	//csrw sip, zero
	la t0, kernel_main
 	li t1, SR_SIE
  	li t2, SIE_SSIE //| SIE_STIE | SIE_SEIE
	//Supervisor_software_interrupt、s_timer_interrupt、s_syscall
	csrw stvec, t0
  	csrw sstatus, t1
  	csrw sie, t2 

wait_for_wakeup:
	wfi
	j wait_for_wakeup

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"

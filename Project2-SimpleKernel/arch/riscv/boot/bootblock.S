#include <asm/sbidef.h>
#include <asm/sbiasm.h>

// #define BOOT_LOADER_SIG_OFFSET 0x1fe
// #define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
// #define OS_SIZE_LOC 0x1fc

.equ os_size_loc_0, 0x502001fa
.equ os_size_loc_1, 0x502001fc

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50201000


// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50201000
 

.text
.global main

main:
	
	/* Load the global pointer */
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop

	
	// fence on all memory and I/O
	fence
	// print a \n to avoid system output error...
	li a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR

	// 1) task1 call BIOS print string "It's bootblock!"
	lui a1, %hi(msg)
	addi a0, a1, %lo(msg) 
	SBI_CALL SBI_CONSOLE_PUTSTR
	
		lui  a0,	%hi(kernel)
		addi a0,a0, %lo(kernel)
		lui  t0,	%hi(os_size_loc_0)
		addi t0,t0, %lo(os_size_loc_0)
		lh	 a1,	0(t0)
		li 	 a2,  	1

		SBI_CALL SBI_SD_READ

		lui  a0,	%hi(kernel_main)
		addi a0,a0, %lo(kernel_main)
		jr	 0(a0)

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"




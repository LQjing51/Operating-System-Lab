#include <asm/sbidef.h>
#include <asm/sbiasm.h>

// #define BOOT_LOADER_SIG_OFFSET 0x1fe
// #define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
// #define OS_SIZE_LOC 0x1fc

.equ os_size_loc_0, 0x502001fa
.equ os_size_loc_1, 0x502001fc

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50200000


// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50200000

// 3.bootblock address
.equ boot,0x50400000

// 3.bootblock remain address
.equ boot_remain,0x50400024

.text
.global main

main:
	// fence on all memory and I/O
	fence

	//move bootblock
	lui  a0,	%hi(boot)
	addi a0,a0, %lo(boot)
	li	 a1,	1
	li 	 a2,  	0
	SBI_CALL SBI_SD_READ
	lui  t0,	%hi(boot_remain)
	addi t0,t0, %lo(boot_remain)
	jr			0(t0)

	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop

	// print a \n to avoid system output error...
	li a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR

	// 1) task1 call BIOS print string "It's bootblock!"
	lui a1, %hi(msg)
	addi a0, a1, %lo(msg) 
	SBI_CALL SBI_CONSOLE_PUTSTR



	// 2) task2 call BIOS read kernel in SD card and jump to kernel start
	

	// choose kernel

	li t0,48
	li t1,49
	li t2,-1
	
	wait:
    	SBI_CALL SBI_CONSOLE_GETCHAR
		beq a0,t2,wait

    bne a0,t0, do_else //kernel0
		j load0
	do_else:
		j load1
	

	load0:
		lui  a0,	%hi(kernel)
		addi a0,a0, %lo(kernel)
		lui  t0,	%hi(os_size_loc_0)
		addi t0,t0, %lo(os_size_loc_0)
		lh	 a1,	0(t0)
		li 	 a2,  	1
		j jump
	
	load1:
		lui  a0,	%hi(kernel)
		addi a0,a0, %lo(kernel)
		lui  t0,	%hi(os_size_loc_0)
		addi t0,t0, %lo(os_size_loc_0)
		lui  t1,	%hi(os_size_loc_1)
		addi t1,t1, %lo(os_size_loc_1)
		lh	 a1,	0(t1)
		lh	 t2,	0(t0)     //kernel0_size
		li a2, 1	//1+kernel0_size
		add a2,a2,t2
		
		j jump

	jump:
		SBI_CALL SBI_SD_READ
		fence.i

		lui  a0,	%hi(kernel_main)
		addi a0,a0, %lo(kernel_main)
		jr	 0(a0)

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"


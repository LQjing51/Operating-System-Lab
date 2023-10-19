#ifndef IOREMAP_H
#define IOREMAP_H
#include<type.h>
#include<pgtable.h>
#include<os/mm.h>
// using this as IO address space (at most using 1 GB, so that it can be store in one pgdir entry)
#define IO_ADDR_START 0xffffffe000000000lu

extern void * ioremap(unsigned long phys_addr, unsigned long size);
extern void iounmap(void *io_addr);
extern void map_page(uint64_t va, uint64_t pa, PTE *pgdir);
extern void map_net_page(uint64_t va, uint64_t pa, PTE *pgdir);
//extern uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir, int from_user);
#endif // IOREMAP_H
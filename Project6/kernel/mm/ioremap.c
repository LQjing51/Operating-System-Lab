#include <os/ioremap.h>
#include <os/mm.h>
#include <pgtable.h>
#include <type.h>

// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;
unsigned long map_count = 0;
void *ioremap(unsigned long phys_addr, unsigned long size)
{
    // map phys_addr to a virtual address
    // then return the virtual address
    unsigned long va_addr;
    unsigned long first_count = map_count;
    while(size > 0){
        va_addr = io_base + map_count*NORMAL_PAGE_SIZE;
        map_count += 1;
        map_net_page(va_addr,phys_addr,pa2kva(PGDIR_PA));
        size -= NORMAL_PAGE_SIZE;
        phys_addr += NORMAL_PAGE_SIZE;
    }
    local_flush_tlb_all();
    return io_base + first_count * NORMAL_PAGE_SIZE;
}

void iounmap(void *io_addr)
{
    // TODO: a very naive iounmap() is OK
    // maybe no one would call this function?
    // *pte = 0;
}

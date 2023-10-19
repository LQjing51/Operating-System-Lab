#include <os/mm.h>
#include <pgtable.h>
#include <sbi.h>
#include <os/sched.h>
#include <assert.h>

int thread_stack_index = 1; 

ptr_t memCurr = FREEMEM;
ptr_t ptCurr  = PTMEM;

#define MEM_NUM 256//20
#define PT_MEM_NUM 256
int fork_mem_shared[MEM_NUM];
int mem_occupied[MEM_NUM];//52000000~52100000  // 1 from me, pid from user
int pt_mem_occupied[PT_MEM_NUM];//56000000~56100000

#define SHARE_PAGE_NUM 20

typedef struct share_mem{
    int key;
    int share_num;
    uintptr_t pa;
}share_mem_t;

share_mem_t share_page[SHARE_PAGE_NUM];

static LIST_HEAD(freePageList);

#define SD_PAGE_NUM 200
ptr_t sd_va[SD_PAGE_NUM];//va + it's pcb's pid in low bits
int sd_page = 0;

int swap(ptr_t pgaddr,int pid,uintptr_t pa){
    PTE *pt_addr  = (PTE *)pa2kva(pgaddr<<NORMAL_PAGE_SHIFT);
    PTE *second_entry;
    PTE *third_entry;
    int i;
    int j;
    int k;
    int sd_index;
    uintptr_t va;
    for(i = 0; i < NUM_PTE_ENTRY; i++){
        if(pt_addr[i]){
            second_entry = (PTE *)pa2kva(get_pa(pt_addr[i]));
            for(j = 0; j < NUM_PTE_ENTRY; j++){
                if(second_entry[j] && (second_entry[j] & (_PAGE_READ | _PAGE_EXEC)) == 0){
                    third_entry = (PTE *)pa2kva(get_pa(second_entry[j]));
                    for (k = 0; k < NUM_PTE_ENTRY; k++){
                        if (third_entry[k]) {
                            if(get_pa(third_entry[k]) == pa){

                                //clear_pgdir(pa2kva(get_pa(third_entry[k])));
                                
                                va = ((uintptr_t)i) << (PPN_BITS*2 + NORMAL_PAGE_SHIFT)
                                    |((uintptr_t)j) << (PPN_BITS + NORMAL_PAGE_SHIFT)
                                    |((uintptr_t)k) << NORMAL_PAGE_SHIFT;
                                va |= (uintptr_t)pid;
                                
                                //sd_va[sd_page++]=va;
                                for(sd_index=1; sd_index<SD_PAGE_NUM; sd_index++){
                                    if(sd_va[sd_index] == 0){
                                        break;
                                    }
                                }
                                assert(sd_index < SD_PAGE_NUM);
                                sd_va[sd_index] = va;
                                third_entry[k]=0;
                                local_flush_tlb_all();
                                return sd_index;
                            }    
                        }
                    }
                }
            }
        }
    }
    return 0;
}
ptr_t allocPage(int numPage, int from_user)
{
    // align PAGE_SIZE
    ptr_t ret = ROUND(memCurr, PAGE_SIZE);
    //memCurr = ret + numPage * PAGE_SIZE;
    int i=0;
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    //pcb_t *tmp = current_running[cpu_id];
    //while(numPage){
        for(;i<MEM_NUM;i++){
            if(!mem_occupied[i]){
                if(from_user){
                    mem_occupied[i]=current_running[cpu_id]->pid;//shell's pid = 1,but shell will not ask for more page
                }else{
                    mem_occupied[i]=1;
                }
                break;
            }
        }
      //  numPage--;
    //}
    if(i<MEM_NUM){
        return memCurr + i*PAGE_SIZE;
    }else{
        int j;
        int count=0;
        pcb_t* src_pcb;
        int pid;
        for(j=0; j < MEM_NUM; j++){
            if(mem_occupied[j] > 1){//pid
                pid = mem_occupied[j];
                for(count=0 ; (pcb[count].pid != pid) && (count < NUM_MAX_TASK) ; count++) ;
                if(count != NUM_MAX_TASK){
                    src_pcb = &pcb[count];
                }
                ptr_t pt_addr = src_pcb -> satp;
                uintptr_t ppn = pt_addr & ((1lu << 44) - 1);
                int sd_index;
                uintptr_t pa = memCurr + j*PAGE_SIZE;
                sd_index = swap(ppn,pid,pa);
                if(!sd_index){
                    prints("failed to search and clean, the ppn is %ld\n",ppn);
                }
                sbi_sd_write(pa,8,(sd_index-1)*8+1);//sd_page will ++ when search_and_clean
                break;
            }
        }
        assert(j < MEM_NUM);
        if(from_user){
            mem_occupied[j]=current_running[cpu_id]->pid;//shell's pid = 1,but shell will not ask for more page
        }else{
            mem_occupied[j]=1;
        }
        return memCurr + j*PAGE_SIZE;
    }
}

void freePage(ptr_t ppn)
{
    // TODO:
    PTE *pt_addr  = (PTE *)pa2kva(ppn<<NORMAL_PAGE_SHIFT);
    PTE *second_entry;
    PTE *third_entry;
    
    int i;
    int j;
    int k;
    int m;
    for(i = 0; i < NUM_PTE_ENTRY; i++){
        if(pt_addr[i]){
            second_entry = (PTE *)pa2kva(get_pa(pt_addr[i]));
            for(j = 0; j < NUM_PTE_ENTRY; j++){

                if(second_entry[j] && (second_entry[j] & (_PAGE_READ | _PAGE_EXEC)) == 0){
                    third_entry = (PTE *)pa2kva(get_pa(second_entry[j]));
                    for (k = 0; k < NUM_PTE_ENTRY; k++){
                        if (third_entry[k] & _PAGE_PRESENT) {
                            //clear_pgdir(pa2kva(get_pa(third_entry[k])));
                            if(fork_mem_shared[(get_pa(third_entry[k])-FREEMEM)/PAGE_SIZE] == 0){
                                for(m=0; m< SHARE_PAGE_NUM; m++){
                                    if(share_page[m].pa == get_pa(third_entry[k])){
                                        break;
                                    }
                                }
                                if(m == SHARE_PAGE_NUM)
                                    mem_occupied[(pa2kva(get_pa(third_entry[k]))-pa2kva(FREEMEM))/PAGE_SIZE] = 0;       
                            
                            
                            }
                        }
                    }
                    //clear_pgdir(third_entry);
                    pt_mem_occupied[((uintptr_t)third_entry-pa2kva(PTMEM))/PAGE_SIZE] = 0;
                    second_entry[j]=0;
                }

            }
            //clear_pgdir(second_entry);//do not delete the kernel address context
        }
    }
    //clear_pgdir(pt_addr);
    pt_mem_occupied[((uintptr_t)pt_addr-pa2kva(PTMEM))/PAGE_SIZE] = 0;
}
//copy pgtable from father
void fork_mem(uintptr_t child_pt,uintptr_t father_pt){
    PTE *pt_addr  = (PTE *)pa2kva(father_pt);
    PTE *second_entry;
    PTE *third_entry;
    int i;
    int j;
    int k;
    uintptr_t va;
    uintptr_t pa;
    for(i = 0; i < NUM_PTE_ENTRY; i++){
        if(pt_addr[i]){
            second_entry = (PTE *)pa2kva(get_pa(pt_addr[i]));
            for(j = 0; j < NUM_PTE_ENTRY; j++){
                if(second_entry[j] && (second_entry[j] & (_PAGE_READ | _PAGE_EXEC)) == 0){
                    third_entry = (PTE *)pa2kva(get_pa(second_entry[j]));
                    for (k = 0; k < NUM_PTE_ENTRY; k++){
                        if (third_entry[k]) {
                                va = ((uintptr_t)i) << (PPN_BITS*2 + NORMAL_PAGE_SHIFT)
                                    |((uintptr_t)j) << (PPN_BITS + NORMAL_PAGE_SHIFT)
                                    |((uintptr_t)k) << NORMAL_PAGE_SHIFT;
                                pa = get_pa(third_entry[k]);
                                fork_mem_shared[(pa-FREEMEM)/PAGE_SIZE]++;
                                map_page(va,pa,(PTE*)pa2kva(child_pt));
                                set_attribute(&third_entry[k], _PAGE_PRESENT | _PAGE_READ | _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_USER | _PAGE_DIRTY);// read_only
                        }    
                    }
                }
            }
        }
    }
    pt_addr  = (PTE *)pa2kva(child_pt);
    for(i = 0; i< NUM_PTE_ENTRY; i++){
        if(pt_addr[i]){
            second_entry = (PTE *)pa2kva(get_pa(pt_addr[i]));
            for(j = 0; j < NUM_PTE_ENTRY; j++){
                if(second_entry[j] && (second_entry[j] & (_PAGE_READ | _PAGE_EXEC)) == 0){
                    third_entry = (PTE *)pa2kva(get_pa(second_entry[j]));
                    for (k = 0; k < NUM_PTE_ENTRY; k++){
                        if (third_entry[k]) {
                                set_attribute(&third_entry[k], _PAGE_PRESENT | _PAGE_READ | _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_USER |_PAGE_DIRTY);// read_only
                                
                        }
                    }
                }
            }
        }
    }
    local_flush_tlb_all();
    
}
void* kmalloc(size_t size)
{
    ptr_t ret = ROUND(memCurr, 4);
    memCurr = ret + size;
    return (void*)memCurr;
}


uintptr_t search_va(uintptr_t pgaddr){
    PTE *pt_addr  = (PTE *)pa2kva(pgaddr);
    PTE *second_entry;
    PTE *third_entry;
    int i;
    int j;
    int k;
    uintptr_t va;
    uintptr_t pa;
    for(i = 0; i < NUM_PTE_ENTRY; i++){
        if(pt_addr[i]){
            second_entry = (PTE *)pa2kva(get_pa(pt_addr[i]));
            for(j = 0; j < NUM_PTE_ENTRY; j++){
                if(second_entry[j] && (second_entry[j] & (_PAGE_READ | _PAGE_EXEC)) == 0){
                    third_entry = (PTE *)pa2kva(get_pa(second_entry[j]));
                    for (k = ((i || j) ? 0 : 1); k < NUM_PTE_ENTRY; k++){
                        if (!third_entry[k]) {
                                va = ((uintptr_t)i) << (PPN_BITS*2 + NORMAL_PAGE_SHIFT)
                                    |((uintptr_t)j) << (PPN_BITS + NORMAL_PAGE_SHIFT)
                                    |((uintptr_t)k) << NORMAL_PAGE_SHIFT;
                                return va;
                        }    
                    }
                }
            }
            for(j = i ? 0 : 1; j < NUM_PTE_ENTRY; j++){
                if(!second_entry[j]){
                    va = ((uintptr_t)i) << (PPN_BITS*2 + NORMAL_PAGE_SHIFT)
                        |((uintptr_t)j) << (PPN_BITS + NORMAL_PAGE_SHIFT)
                        |((uintptr_t)0) << NORMAL_PAGE_SHIFT;
                    return va;
                }
            }
        }

    }
    for(i = 1;i < NUM_PTE_ENTRY; i++){// do not use va=0 
        if(!pt_addr[i]){
            va = ((uintptr_t)i) << (PPN_BITS*2 + NORMAL_PAGE_SHIFT)
                |((uintptr_t)0) << (PPN_BITS + NORMAL_PAGE_SHIFT)
                |((uintptr_t)0) << NORMAL_PAGE_SHIFT;
            return va;
        }
    }
    return 0;
}
uintptr_t search_share_pa(uintptr_t pgaddr,uintptr_t va){
    va &= VA_MASK;
    uint64_t vpn2 =
        va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = (vpn1 << PPN_BITS) ^ (vpn2 << (PPN_BITS + PPN_BITS)) ^ 
                    (va >> (NORMAL_PAGE_SHIFT));
    PTE *pt_addr  = (PTE *)pa2kva(pgaddr);
    PTE *second_entry;
    PTE *third_entry;
    uintptr_t pa;
    uintptr_t new_page_kva;
    if(pt_addr[vpn2]){
        second_entry = (PTE *)pa2kva(get_pa(pt_addr[vpn2]));
        if(second_entry[vpn1]){
            third_entry = (PTE *)pa2kva(get_pa(second_entry[vpn1]));
            if(third_entry[vpn0] & _PAGE_PRESENT){//page_present
                pa =  get_pa(third_entry[vpn0]);
                third_entry[vpn0] = 0;
                return pa;
            }
        }
    }
    local_flush_tlb_all();
    return 0;    
}
uintptr_t shm_page_get(int key)
{
    // TODO(c-core):
    uintptr_t new_va;
    uintptr_t new_kva;
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    pcb_t *now_pcb = current_running[cpu_id];
    ptr_t satp = now_pcb -> satp;
    uintptr_t pt_addr = (satp & ((1lu << 44) - 1)) << NORMAL_PAGE_SHIFT;
    int i;
    for(i=0; i< SHARE_PAGE_NUM; i++){
        if(share_page[i].key == key){
            new_va = search_va(pt_addr);
            share_page[i].share_num++;
            map_page(new_va, share_page[i].pa, (PTE*)pa2kva(pt_addr));
            return new_va;
        }
    }
    for(i=0; i<SHARE_PAGE_NUM; i++){
        if(share_page[i].key == 0){
            break;
        }
    }
    assert(i<SHARE_PAGE_NUM);
    share_page[i].key = key;
    share_page[i].share_num ++;
    new_va = search_va(pt_addr);
    new_kva = alloc_page_helper((uintptr_t)(new_va),pt_addr,0);
    share_page[i].pa = kva2pa(new_kva);
    return new_va;

}

void shm_page_dt(uintptr_t addr)
{
    // TODO(c-core):
    uintptr_t pa;
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    pcb_t *now_pcb = current_running[cpu_id];
    ptr_t satp = now_pcb -> satp;
    uintptr_t pt_addr = (satp & ((1lu << 44) - 1)) << NORMAL_PAGE_SHIFT;
    pa = search_share_pa(pt_addr,addr);//include cleaning the corresponding third page table
    for(int i; i<SHARE_PAGE_NUM; i++){
        if(share_page[i].pa == pa){
            share_page[i].share_num--;
            if(share_page[i].share_num == 0){
                mem_occupied[(share_page[i].pa - FREEMEM)/PAGE_SIZE] = 0;
                share_page[i].pa = 0;
                share_page[i].key = 0;
            }
            break;
        }
    }
}

void map_page(uint64_t va, uint64_t pa, PTE *pgdir)
{
    va &= VA_MASK;
    uint64_t vpn2 =
        va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = (vpn1 << PPN_BITS) ^ (vpn2 << (PPN_BITS + PPN_BITS)) ^ 
                    (va >> (NORMAL_PAGE_SHIFT));     
    if (pgdir[vpn2] == 0) {
        // alloc a new second-level page directory
        set_pfn(&pgdir[vpn2], create_pt() >> NORMAL_PAGE_SHIFT);
        set_attribute(&pgdir[vpn2], _PAGE_PRESENT);
        //clear_pgdir(get_pa(pgdir[vpn2]));
    }
    PTE *second_pgdir = (PTE *) pa2kva(get_pa(pgdir[vpn2]));
    if (second_pgdir[vpn1] == 0) {
        set_pfn(&second_pgdir[vpn1], create_pt() >> NORMAL_PAGE_SHIFT);
        set_attribute(&second_pgdir[vpn1], _PAGE_PRESENT);
        //clear_pgdir(pa2kva(get_pa(second_pgdir[vpn1])));
    }
    PTE *last_pgdir = (PTE *) pa2kva(get_pa(second_pgdir[vpn1]));
    set_pfn(&last_pgdir[vpn0], pa >> NORMAL_PAGE_SHIFT);
    set_attribute(
        &last_pgdir[vpn0], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | _PAGE_USER |
                        _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY);
}

/* this is used for mapping kernel virtual address into user page table */
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    // TODO:
    PTE *dest_addr = pa2kva(dest_pgdir);
    PTE *src_addr  = pa2kva(src_pgdir );//PGDIR_PA
    for (int i = 0; i < NUM_PTE_ENTRY; i++) {
        if (*src_addr) {
            *dest_addr = *src_addr;
        }
        src_addr++;
        dest_addr++;
    }
}
int check_replaced(uintptr_t va){
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    //int pid = current_running[cpu_id]->pid
    pcb_t *now_pcb = current_running[cpu_id];
    int pid = now_pcb -> pid;
    ptr_t pt_addr = now_pcb -> satp;
    uintptr_t ppn = pt_addr & ((1lu << 44) - 1);

    uintptr_t vpn = va & ~((1lu << NORMAL_PAGE_SHIFT) - 1);
    vpn |= pid;
    int i;
    for(i=0; i<SD_PAGE_NUM; i++){
        if(sd_va[i] == vpn){
            ptr_t addr = allocPage(1,1);//from_user
            map_page(va, addr, (PTE*)pa2kva(ppn<<NORMAL_PAGE_SHIFT));
            sd_va[i] = 0;
            sbi_sd_read(addr,8,(i-1)*8+1);
            return addr;
        }
    }
    return 0;
}
ptr_t check_fork(uintptr_t va,uintptr_t pgdir){

    va &= VA_MASK;
    uint64_t vpn2 =
        va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = (vpn1 << PPN_BITS) ^ (vpn2 << (PPN_BITS + PPN_BITS)) ^ 
                    (va >> (NORMAL_PAGE_SHIFT));
    PTE *pt_addr  = (PTE *)pa2kva(pgdir);
    PTE *second_entry;
    PTE *third_entry;
    
    uintptr_t pa;
    uintptr_t new_page_kva;
    if(pt_addr[vpn2]){
        second_entry = (PTE *)pa2kva(get_pa(pt_addr[vpn2]));
        if(second_entry[vpn1]){
            third_entry = (PTE *)pa2kva(get_pa(second_entry[vpn1]));
            if(third_entry[vpn0] & _PAGE_PRESENT){//page_present
                pa = get_pa(third_entry[vpn0]);
                if(fork_mem_shared[(pa-FREEMEM)/PAGE_SIZE]){
                    fork_mem_shared[(pa-FREEMEM)/PAGE_SIZE]--;
                    new_page_kva = alloc_page_helper(va,pgdir,0);
                    kmemcpy(new_page_kva,pa2kva(pa),PAGE_SIZE);
                    return kva2pa(new_page_kva);
                }else{
                    set_attribute(&third_entry[vpn0], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |  _PAGE_EXEC | _PAGE_USER | _PAGE_ACCESSED | _PAGE_DIRTY);
                    return pa;
                }   
            }
        }
    }
    return 0;    
}
/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page.
   */
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir, int from_user)
{
    // TODO:
    ptr_t fork_add_page;
    if(from_user){
        fork_add_page = check_fork(va,pgdir);
        if(fork_add_page){
            return pa2kva(fork_add_page);
        }
    }
    if(from_user){
        ptr_t replaced_addr = check_replaced(va);
        if(replaced_addr){
            return pa2kva(replaced_addr);
        }
    }
    ptr_t addr = allocPage(1,from_user);

    map_page(va, addr, (PTE*)pa2kva(pgdir));
    return pa2kva(addr);
}

uintptr_t create_pt() {
    ptr_t ret = ROUND(ptCurr, PAGE_SIZE);

    int i=0;
    for(;i<PT_MEM_NUM;i++){
        if(!pt_mem_occupied[i]){
            pt_mem_occupied[i]=1;
            ret = ptCurr + i*PAGE_SIZE;
            clear_pgdir(pa2kva(ret));
            return ret;
        }
    }
    //return ret;
} 

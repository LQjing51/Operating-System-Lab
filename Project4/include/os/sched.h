/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking,
 *                 process wakeup, process creation, process kill, etc.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_
 
#include <context.h>

#include <type.h>
#include <os/list.h>
#include <os/mm.h>
#include <os.h>
#include <context.h>

#define NUM_MAX_TASK 16
#define NUM_MAX_FREE 30
/* used to save register infomation */


extern pid_t do_exec(const char* file_name, int argc, char* argv[], spawn_mode_t mode);
extern void do_show_exec();


typedef enum {
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_ZOMBIE,
    TASK_EXITED,
} task_status_t;

/*typedef enum {
    ENTER_ZOMBIE_ON_EXIT,
    AUTO_CLEANUP_ON_EXIT,
} spawn_mode_t;

typedef enum {
    KERNEL_PROCESS,
    KERNEL_THREAD,
    USER_PROCESS,
    USER_THREAD,
} task_type_t;*/

/* Process Control Block */
typedef struct pcb
{
    /* register context */
    // this must be this order!! The order is defined in regs.h
    reg_t kernel_sp;
    reg_t user_sp;
    reg_t satp;
    // count the number of disable_preempt
    // enable_preempt enables CSR_SIE only when preempt_count == 0

    list_node_t list;
    list_head wait_list;

    ptr_t kernel_stack_base;
    ptr_t user_stack_base;

    /* previous, next pointer */
    //list_node_t list;
    //list_head wait_list;

    /* process id */
    pid_t pid;
    uint64_t cpu;
    int mask;
    int lock[5];
    int killed;
    int need_clean;
    int is_thread;
    //int father;
    //int child[5];
    //int barrier[5];
    //int semaphora[5];
    /* kernel/user thread/process */
    task_type_t type;

    /* BLOCK | READY | RUNNING | ZOMBIE */
    task_status_t status;
    spawn_mode_t mode;

    /*wake up*/
    reg_t wakeup;
    
  //  reg_t priority;
   // reg_t tmp_priority;
    /* cursor position */
    int cursor_x;
    int cursor_y;
} pcb_t;

/* task information, used to init PCB */
/*typedef struct task_info
{
    ptr_t entry_point;
    task_type_t type;
  //  reg_t priority;
} task_info_t;*/

/* ready queue to run */
extern list_head ready_queue;
//extern list_head block_queue;
/* current running task PCB */
extern pcb_t * volatile current_running[2];
// extern pcb_t * volatile current_running[NR_CPUS];
extern pid_t process_id;
extern ptr_t free_space[NUM_MAX_FREE];
//extern int free_index;
extern pcb_t pcb[NUM_MAX_TASK];
// extern pcb_t kernel_pcb[NR_CPUS];
extern pcb_t pid0_pcb_s;
extern pcb_t pid0_pcb_m;
extern const ptr_t pid0_stack;
extern uint32_t get_time(uint32_t *time_elapsed);
extern void in_queue(list_head* queue, pcb_t* pcb);
extern void out_queue(pcb_t* pcb);
extern void list_init(list_head* queue);

extern void switch_to(pcb_t *prev, pcb_t *next);
extern void do_scheduler(void);
extern void do_sleep(uint32_t);

//extern pid_t do_spawn(task_info_t *task, void* arg, spawn_mode_t mode);
extern void do_exit(void);
extern int do_kill(pid_t pid);
extern int do_waitpid(pid_t pid);
extern void do_process_show();
extern pid_t do_getpid();
extern pid_t do_exec(const char* file_name, int argc, char* argv[], spawn_mode_t mode);
extern void do_show_exec();
 
extern void do_block(pcb_t *pcb_node, list_head *queue);
extern void do_unblock(list_node_t *);

extern reg_t do_fork();
extern int mthread_create(void (*start_routine)(void*), void *arg);
extern int mthread_join(pid_t thread);
#endif

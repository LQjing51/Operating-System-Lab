#ifndef OS_H
#define OS_H

#include <stdint.h>
//extern enum spawn_mode_t;
//extern enum task_type_t;
//extern struct task_info task_info_t;
typedef enum {
    ENTER_ZOMBIE_ON_EXIT,
    AUTO_CLEANUP_ON_EXIT,
} spawn_mode_t;

typedef enum {
    KERNEL_PROCESS,
    KERNEL_THREAD,
    USER_PROCESS,
    USER_THREAD,
} task_type_t;

/* task information, used to init PCB */
typedef struct task_info
{
    uintptr_t entry_point;
    task_type_t type;
} task_info_t;

typedef int32_t pid_t;

#endif // OS_H

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                       System call related processing
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

#ifndef INCLUDE_SYSCALL_H_
#define INCLUDE_SYSCALL_H_

#include <os/syscall_number.h>
#include <stdint.h>
#include <type.h>
#include <os.h>
 
#define SCREEN_HEIGHT 80

#define BINSEM_OP_LOCK 0 // mutex acquire
#define BINSEM_OP_UNLOCK 1 // mutex release

pid_t sys_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode);
void sys_show_exec();
extern long invoke_syscall(long, long, long, long, long);

extern void sys_process_show(void);
extern void sys_screen_clear(void);
extern void sys_back_cursor(void);

extern pid_t sys_spawn(task_info_t *info, void* arg, spawn_mode_t mode);
extern void sys_exit(void);
extern int sys_kill(pid_t pid);
extern int sys_waitpid(pid_t pid);
extern pid_t sys_getpid();

extern void sys_yield();
extern void sys_sleep(uint32_t time);

extern void sys_futex_wait(volatile uint64_t *val_addr, uint64_t val);
extern void sys_futex_wakeup(volatile uint64_t *val_addr, int num_wakeup);


extern void sys_write(char *);//
extern char sys_read();
extern void sys_move_cursor(int, int);//
extern void sys_reflush();

extern long sys_get_timebase();
extern long sys_get_tick();

//int binsemget(int key);
//int binsemop(int binsem_id, int op);

extern int sys_get_char();

pid_t sys_create_thread(uintptr_t entry_point, void* arg);
int sys_mbox_open(char *name);
void sys_mbox_close(int mbox_id);
void sys_mbox_send(int mbox_id, void *msg, int msg_length);
void sys_mbox_recv(int mbox_id, void *msg, int msg_length);

extern int binsemget(void* handle, int key);
extern int binsemop(int id,int op); 
int sys_fork();

long sys_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength);
void sys_net_send(uintptr_t addr, size_t length);
void sys_net_irq_mode(int mode);

extern int sys_ln(char* dir,char* src);
extern int sys_ls_l(char* filename);
extern int sys_rm(char* filename);
extern int sys_lseek(int fd,int offset,int whence);
extern int sys_touch(char* filename);
extern int sys_cat(char* filename);
extern int sys_mkfs();
extern int sys_statfs();
extern int sys_cd(char* directories);
extern int sys_mkdir(char* directory);
extern int sys_rmdir(char* directory);
extern int sys_ls(char* directories);
extern int sys_fopen(int filename,int mode);
extern int sys_fwrite(int fd, char* context, int length);
extern int sys_fread(int fd, char *buff, int length);
extern int sys_close(int fd);


#endif

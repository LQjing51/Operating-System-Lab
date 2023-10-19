/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode.
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
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

//#include <test.h>
#include <test_project3/test3.h>
#include <string.h>
#include <os.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>

struct task_info task_test_waitpid = {
    (uintptr_t)&wait_exit_task, USER_PROCESS};
struct task_info task_test_semaphore = {
    (uintptr_t)&semaphore_add_task1, USER_PROCESS};
struct task_info task_test_barrier = {
    (uintptr_t)&test_barrier, USER_PROCESS};
    
struct task_info strserver_task = {(uintptr_t)&strServer, USER_PROCESS};
struct task_info strgenerator_task = {(uintptr_t)&strGenerator, USER_PROCESS};

//struct task_info bi_spawn_task = {(uintptr_t)&bi_mailbox_spawn, USER_PROCESS};
struct task_info bi_mailbox_task = {(uintptr_t)&bi_mailbox, USER_PROCESS};
//struct task_info bi_strgenerator_task = {(uintptr_t)&strGenerator, USER_PROCESS};

struct task_info task_test_multicore = {(uintptr_t)&test_multicore, USER_PROCESS};
struct task_info task_test_affinity = {(uintptr_t)&test_affinity, USER_PROCESS};

static struct task_info *test_tasks[16] = {&task_test_waitpid,//0
                                           &task_test_semaphore,//1
                                           &task_test_barrier,//2
                                           &task_test_multicore,//3
                                           &strserver_task, //4
                                           &strgenerator_task,//5
                                           &bi_mailbox_task,//6
                                           &task_test_affinity};//7
static int num_test_tasks = 8;
#define SHELL_BEGIN 12

void parse(char * buffer){
    char command[3][10];
    int i=0;
    int j=0;
    while(buffer[i]!='\0' && buffer[i]!=' '){
        command[0][j++]=buffer[i++];
    }
    command[0][j]='\0';

    j=0;
    i++;
    int num1=0;int index1=0;
    if(buffer[i-1]==' ' && buffer[i] >= '0' && buffer[i] <= '9'){
        while(buffer[i]!='\0' && buffer[i]!=' '){
            command[1][j++]=buffer[i++];
        }
        command[1][j]='\0';
        for(index1=0 ; index1 < j ; index1++){
            num1 = num1*10 + (command[1][index1]-'0');
        }
       // printf("num1=%n",num1);
    }

    j=0;
    i++;
    int num2=0;int index2=0;
     if(buffer[i-1]==' ' && buffer[i] >= '0' && buffer[i] <= '9'){
        while(buffer[i]!='\0' && buffer[i]!=' '){
            command[2][j++]=buffer[i++];
        }
        command[2][j]='\0';
        for(index2=0 ; index2 < j ; index2++){
            num2 = num2*10 + (command[2][index2]-'0');
        }
       // printf("num2=%d\n",num2);
    }

    char taskset[20];
    i=0;
    j=0;
    while(buffer[i]!='\0' && (buffer[i]<'0' || buffer[i]>'9')){
        taskset[j++]=buffer[i++];
    }
    taskset[j]='\0';

    if(!strcmp(command[0],"ps")){
        sys_process_show();
    }else if(!strcmp(command[0],"clear")){
        sys_screen_clear();
        sys_move_cursor(1,SHELL_BEGIN);
        printf("------------------- COMMAND -------------------\n");
    }else if(!strcmp(command[0],"exec")){
        pid_t pid;
        if(num2==1){ 
            pid = sys_spawn(test_tasks[num1],NULL,AUTO_CLEANUP_ON_EXIT);
        }else if(num2==2){
            pid = sys_spawn(test_tasks[num1],NULL,ENTER_ZOMBIE_ON_EXIT);
        }
        if(pid!=0){
            printf("exec process[%d]\n",pid);
        }else{
            printf("wrong input\n");
        }
    }else if(!strcmp(command[0],"kill")){
        if(sys_kill(num1)){
            printf("kill process[%d]\n",num1);
        }else{
            printf("failed to kill process[%d]:no process[%d]\n",num1,num1);
        }
    }else if(!strcmp(command[0],"exit")){
        sys_exit();
    }else if(!strcmp(command[0],"wait")){
        if(sys_waitpid(num1)){
            printf("wait for process[%d]\n",num1);
        }else{
            printf("failed to wait for process[%d]:no process[%d]\n",num1,num1);
        }
    }else if(!strcmp(taskset,"taskset ")){
        int mask = buffer[i+2]-'0';
        int num = buffer[i+4]-'0';
        pid_t pid;
        //printf("this is taskset %d %d\n",mask,num);
        pid = sys_taskset(test_tasks[num],mask);
        printf("exec and set mask process[%d]\n",pid);
    }else if(!strcmp(taskset,"taskset -p ")){
        int mask = buffer[i+2]-'0';
        int pid = buffer[i+4]-'0';
        //printf("this is taskset -p %d %d\n",mask,pid);
        sys_taskset_p(pid,mask);
        printf("set mask process[%d]\n",pid);
    }else{
        printf("Unknown command\n");
    }
}


void test_shell()
{
    // TODO:
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
   // sys_move_cursor(1, SHELL_BEGIN+1);
    printf("> root@UCAS_OS:");
   // printf(" ");
    char buffer[30];
    int i=0;
    char get_char;
    while (1)
    {
        //sys_move_cursor(1, 16);
        //printf("in shell %d",i++);
        //printf(" ");
        // TODO: call syscall to read UART port
        get_char = sys_read();
        /*if ((get_char > 'a') && (get_char < 'z')) {
            //sys_move_cursor(1, 13);
            //buffer[i++] = get_char;
            printf("%c",get_char);
        }*/
        if(get_char == '\r' || get_char == '\n'){
            //printf("")
            printf("\n");
            buffer[i] = '\0';
            //printf("%s",buffer);
            parse(buffer);
            i = 0;
            printf("> root@UCAS_OS: ");
        //   sys_yield();
        }else if( get_char == 8 || get_char == 127){
            sys_back_cursor();
            i--;
        }else if (((get_char >= 'a') && (get_char <= 'z')) || ((get_char >= '0') && (get_char <= '9')) || (get_char == 32) || (get_char == '-')) {
            //sys_move_cursor(1, 13);
            buffer[i++] = get_char;
            printf("%c",get_char);
        }
        // TODO: parse input
        // note: backspace maybe 8('\b') or 127(delete)

        // TODO: ps, exec, kill, clear
       // sys_yield();
    }
}

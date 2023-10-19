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

#include <string.h>
#include <os.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>

#define SHELL_BEGIN 13

void parse(char * buffer){
    //parameter 1
    char command[4][50];
    command[0][0] = 0;
    command[1][0] = 0;
    command[2][0] = 0;
    command[3][0] = 0;
    int i=0;
    int j=0;
    int ret=0;
    while(buffer[i]!='\0' && buffer[i]!=' '){
        command[0][j++]=buffer[i++];
    }
    command[0][j]='\0';

    j=0;
    i++;
    int num1=0;int index1=0;
    if(buffer[i-1]==' '){
        while(buffer[i]!='\0' && buffer[i]!=' '){
            command[1][j++]=buffer[i++];
        }
        command[1][j]='\0';
        if(command[1][j-1] >= '0' && command[1][j-1] <= '9'){
            for(index1=0 ; index1 < j ; index1++){
                num1 = num1*10 + (command[1][index1]-'0');
            }
        }
       // printf("num1=%n",num1);
    } else command[1][0] = 0;

    j=0;
    i++;
    int num2=0;int index2=0;
     if(buffer[i-1]==' '){
        while(buffer[i]!='\0' && buffer[i]!=' '){
            command[2][j++]=buffer[i++];
        }
        command[2][j]='\0';
        if(command[2][j-1] >= '0' && command[2][j-1] <= '9'){
            for(index2=0 ; index2 < j ; index2++){
                num2 = num2*10 + (command[2][index2]-'0');
            }
        }
       // printf("num2=%d\n",num2);
    }  else command[2][0] = 0;

    j=0;
    i++;
    int num3=0;int index3=0;
     if(buffer[i-1]==' '){
        while(buffer[i]!='\0' && buffer[i]!=' '){
            command[3][j++]=buffer[i++];
        }
        command[3][j]='\0';
        if(command[3][j-1] >= '0' && command[3][j-1] <= '9'){
            for(index3=0 ; index3 < j ; index3++){
                num3 = num3*10 + (command[3][index3]-'0');
            }
        }
       // printf("num2=%d\n",num2);
    }  else command[3][0] = 0;

    char taskset[20];
    i=0;
    j=0;
    while(buffer[i]!='\0' && (buffer[i]<'0' || buffer[i]>'9')){
        taskset[j++]=buffer[i++];
    }
    taskset[j]='\0';//taskset or taskset -p

    char exec[20];
    int ex_index=0;
    j=0;
    while(buffer[ex_index]!='\0' && buffer[ex_index] != ' '){
        exec[j++]=buffer[ex_index++];
    }
    exec[j]='\0';//exec[]="exec"

    char exec_name[20];
    ++ex_index;
    j=0;
    while(buffer[ex_index]!='\0' && buffer[ex_index] != ' '){
        exec_name[j++]=buffer[ex_index++];
    }
    exec_name[j]='\0';

    if(!strcmp(command[0],"ps")){
        sys_process_show();
    }else if(!strcmp(command[0],"clear")){
        sys_screen_clear();
        sys_move_cursor(1,SHELL_BEGIN);
        printf("------------------- COMMAND -------------------\n");
    }else if(!strcmp(command[0],"exec")){
        pid_t pid;
        int argc; 
        char* argv[10];
        int count = 0;
        int m=4;//begin from the word after exec
        while(buffer[m]!='\0'){
            if(buffer[m]==' '){
                argv[count]=&buffer[m+1];
                buffer[m]=0;
                count++;
            }
            m++;
        }
        argc = count;
            pid = sys_exec (exec_name,argc,argv,AUTO_CLEANUP_ON_EXIT);
        
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
    }else if(!strcmp(command[0],"mkfs")){
        ret = sys_mkfs();
        if(ret){
            printf("mkfs success!\n");
        }else{
            printf("mkfs error!\n");
        }
    }else if(!strcmp(command[0],"mkdir")){
        ret = sys_mkdir(command[1]);//filename
        if(ret){
            printf("mkdir success!\n");
        }else{
            printf("mkdir error!\n");
        }
    }else if(!strcmp(command[0],"rmdir")){
        ret = sys_rmdir(command[1]);//filename
        if(ret){
            printf("rmdir success!\n");
        }else{
            printf("rmdir error!\n");
        }
    }else if(!strcmp(command[0],"ls")){
        if(!strcmp(command[1],"-l")){
            //ls -l
            ret = sys_ls_l(command[2]);
            if(ret){
                printf("ls -l success!\n");
            }else{
                printf("ls -l error!\n");
            }
        }else{
            //ls
            ret = sys_ls(command[1]);
            if(ret){
                printf("ls success!\n");
            }else{
                printf("ls error!\n");
            }
        }
    }else if(!strcmp(command[0],"statfs")){
        ret = sys_statfs();
        if(ret){
            printf("statfs success!\n");
        }else{
            printf("statfs error!\n");
        }
    }else if(!strcmp(command[0],"cd")){
        ret = sys_cd(command[1]);
        if(ret){
            printf("cd success!\n");
        }else{
            printf("cd error!\n");
        }
    }else if(!strcmp(command[0],"touch")){
        ret = sys_touch(command[1]);//filename
        if(ret){
            printf("touch success!\n");
        }else{
            printf("touch error!\n");
        }
    }else if(!strcmp(command[0],"cat")){
        ret = sys_cat(command[1]);//filename
        if(ret){
            printf("cat success!\n");
        }else{
            printf("cat error!\n");
        }
    }else if(!strcmp(command[0],"ln")){
        ret = sys_ln(command[1],command[2]);//filename
        if(ret){
            printf("ln success!\n");
        }else{
            printf("ln error!\n");
        }
    }else if(!strcmp(command[0],"rm")){
        ret = sys_rm(command[1]);//filename
        if(ret){
            printf("rm success!\n");
        }else{
            printf("rm error!\n");
        }
    }else if(!strcmp(command[0],"lseek")){
        ret = sys_lseek(num1,num2,num3);
        //int fd,int offset,int whence
        if(ret){
            printf("lseek success!\n");
        }else{
            printf("lseek error!\n");
        }
    }
    
    /*else if(!strcmp(command[0],"wait")){
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
    }*/else{
        printf("Unknown command\n");
    }
}


void main()
{
    // TODO:
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
   // sys_move_cursor(1, SHELL_BEGIN+1);
    printf("> root@UCAS_OS: ");
   // printf(" ");
    char buffer[50];
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
        }else if (((get_char >= 'a') && (get_char <= 'z')) || ((get_char >= '0') && (get_char <= '9')) || (get_char == 32) || (get_char == '-') || (get_char == '/') || (get_char == '.')) {
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

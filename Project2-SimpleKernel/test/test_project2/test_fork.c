#include <stdio.h>
#include <sys/syscall.h>
#include <test2.h>
void fork_task(void)
{
   int print_location = 9;
   int i;
  // int fpid = 0;
   int child = 0;
   int fork_able = 1;
   char priority;
   int tmp;

   for (i = 0;; i++){
        sys_move_cursor(1, print_location);
        if(!child){ 
            printf("> [TASK] This is father thread. (%d)", i);
        }else{
            printf("> [TASK] This is child thread. (%d)", i);
        }
        if(!fork_able){
            continue;
        }
        //char priority;
        //int tmp;
        priority=sys_read();
        while(priority<'0'||priority>'9'){
            priority=sys_read();
        }
        tmp=priority-'0';
        if(sys_fork(tmp)){
            fork_able = 0;
        }else{
           child++;
           print_location++;
        }   
   }
    /*for(i=0;;i++){
        sys_move_cursor(1, print_location);
        printf("> [TASK] This is father thread. (%d)", i);
        priority=sys_read();
        while(priority<'0'||priority>'9'){
            priority=sys_read();
        }
        tmp=priority-'0';
        if(sys_fork(tmp)){
           while(1){
               sys_move_cursor(1, print_location);
               printf("> [TASK] This is father thread. (%d)", i);
               i++;
           }
        }else{
            print_location++;
           while(1){
               sys_move_cursor(1, print_location);
               printf("> [TASK] This is child thread. priority is %d,(%d)",tmp, i);
               i++;
           }
        }   
    }*/
}

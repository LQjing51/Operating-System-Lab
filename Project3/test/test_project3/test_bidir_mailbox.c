#include <time.h>
#include <test_project3/test3.h>
#include <mthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailbox.h>
#include <sys/syscall.h>

void mail_task(int print_location);
#define NUM_TB 3
void bi_mailbox(void){

    mailbox_t mailbox1 = mbox_open("mailbox1");
    mailbox_t mailbox2 = mbox_open("mailbox2");
    mailbox_t mailbox3 = mbox_open("mailbox3");

    struct task_info child_task = {(uintptr_t)&mail_task,
                                   USER_PROCESS};
    pid_t pids[NUM_TB];
    for (int i = 0; i < NUM_TB; ++i) {
        pids[i] = sys_spawn(&child_task, (void*)(i + 1),
                            ENTER_ZOMBIE_ON_EXIT);
    }

    for (int i = 0; i < NUM_TB; ++i) {
        sys_waitpid(pids[i]);
    }
    sys_exit();
}
void generateString(char* buf, int len)
{
    static const char alpha[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-={};|[],./<>?!@#$%^&*";
    static const int alphaSz = sizeof(alpha) / sizeof(char) - 1 ;
    int i = len - 2;
    buf[len - 1] = '\0';
    while (i >= 0) {
        buf[i] = alpha[rand() % alphaSz + 1];
        --i;
    }
}
int count_byte(char* buf){
    int i=0;
    char *tmp = buf;
    while(*tmp){
        i++;
        tmp++;
    }
    return i;
}
void mail_task(int print_location)
{
    char send_Buffer[MAX_MBOX_LENGTH] = {0};
    int send_len;

    char recv_Buffer[MAX_MBOX_LENGTH] = {0};
    int recv_len;
    int to;
    int result;
    mailbox_t index[2];
    mailbox_t always_recv[2];
    mailbox_t me;
    mailbox_t opp;
    if(print_location == 1){ 
        me = mbox_open("mailbox1");
    }else if(print_location == 2){
        me = mbox_open("mailbox2");
    }else{
        me = mbox_open("mailbox3");
    }

    for (;;)
    {
        recv_Buffer[0] = '\0';
        send_Buffer[0] = '\0';
        //send
        to = rand() % 3 + 1;
        while(to == me){
            to = rand() % 3 + 1;
        }
        if(to == 1){ 
            opp = mbox_open("mailbox1");
        }else if(to == 2){
            opp = mbox_open("mailbox2");
        }else{
            opp = mbox_open("mailbox3");
        }
        index[0] = me;
        index[1] = opp;

        sys_move_cursor(1, print_location);
        printf("send to: %ld   recv from: %ld     ",
              opp, me);
        send_len = (rand() % (MAX_MBOX_LENGTH/4)) + 1;
       // printf("me: %d   to: %d    send_len=%d",me, to,send_len);
        generateString(send_Buffer, send_len);

        result = send_recv(index,send_Buffer,recv_Buffer);//result == 0 :finish send and recv
        if(result == 1){
            //recv failed
            index[0]=me;
            index[1]=0;
            //while(result==1){
                result = send_recv(index,NULL,recv_Buffer);
            //}
        }else if(result == 2){
            //send failed
            index[0]=0;
            index[1]=opp;
            //always_recv[0]=me;
            //always_recv[1]=0;
            //while(result==2){
                result = send_recv(index,send_Buffer,NULL);
              //  send_recv(always_recv,NULL,recv_Buffer);
           //}

        }
        recv_len = strlen(recv_Buffer);

        //sys_move_cursor(1, print_location);
        printf("recved Bytes: %ld   send Bytes: %ld             ",
              recv_len, send_len);
        sys_sleep(1);
    }

    sys_exit();
}


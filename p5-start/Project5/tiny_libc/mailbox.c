#include <mailbox.h>
#include <string.h>
#include <sys/syscall.h>

mailbox_t mbox_open(char *name)
{
    // TODO:
    return invoke_syscall(SYSCALL_MBOX_OPEN, name, IGNORE, IGNORE,IGNORE);
}

void mbox_close(mailbox_t mailbox)
{
    // TODO:
    invoke_syscall(SYSCALL_MBOX_CLOSE, mailbox, IGNORE, IGNORE,IGNORE);
}

int mbox_send(mailbox_t mailbox, void *msg, int msg_length)
{
    // TODO:
    return invoke_syscall(SYSCALL_MBOX_SEND, mailbox, msg, msg_length,IGNORE);
}   

int mbox_recv(mailbox_t mailbox, void *msg, int msg_length)
{
    // TODO:
    return invoke_syscall(SYSCALL_MBOX_RECV, mailbox, msg, msg_length,IGNORE);
}
int send_recv(mailbox_t* index, void *send_msg, void* recv_msg){
    return invoke_syscall(SYSCALL_SEND_RECV, index, send_msg, recv_msg,IGNORE);
}
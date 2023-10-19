#ifndef NET_H
#define NET_H

#include <type.h>
#include <os/sched.h>
long do_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength);
void do_net_send(uintptr_t addr, size_t length);
void do_net_irq_mode(int mode);
extern void net_check();
extern int net_poll_mode;
extern list_head wait_recv_queue;
extern list_head wait_send_queue;
#endif //NET_H

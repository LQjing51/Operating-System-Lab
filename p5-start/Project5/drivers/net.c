#include <net.h>
#include <os/string.h>
#include <screen.h>
#include <emacps/xemacps_example.h>
#include <emacps/xemacps.h>

#include <os/sched.h>
#include <os/mm.h>

EthernetFrame rx_buffers[RXBD_CNT];
EthernetFrame tx_buffer;
uint32_t rx_len[RXBD_CNT];

int net_poll_mode;

volatile int rx_curr = 0, rx_tail = 0;
LIST_HEAD(wait_recv_queue);
LIST_HEAD(wait_send_queue);
//extern void do_block(pcb_t *pcb_node, list_head *queue);
void net_check(){
    //send compelete
    // printk("net check %lx\n", EmacPsInstance.Config.BaseAddress);
    u32 TXSR_reg = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress,XEMACPS_TXSR_OFFSET);
    // printk("txsr %lu\n",TXSR_reg);
    if(TXSR_reg & XEMACPS_TXSR_TXCOMPL_MASK){
        release_queue(&wait_send_queue);
    }
    
    //recv complete
    u32 RXSR_reg = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress,XEMACPS_RXSR_OFFSET);
    if(RXSR_reg & XEMACPS_RXSR_FRAMERX_MASK){
        release_queue(&wait_recv_queue);
    }

}
long do_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength)
{
    // TODO: 
    long ret;
    int has_recv = 0;
    int i;
    int remain;
    uint8_t* user_addr;
    user_addr = (uint8_t*)addr;
    remain = num_packet;
    while(remain){ 
        if(remain <= RXBD_CNT){
            ret = EmacPsRecv(&EmacPsInstance,rx_buffers,remain);          
            ret = EmacPsWaitRecv(&EmacPsInstance,remain,rx_len);
            for(i = 0; i < remain; i++){
                memcpy(user_addr,&rx_buffers[i],rx_len[i]);
                user_addr += rx_len[i];
                frLength[num_packet-remain+i] = rx_len[i];
            }
            break;
        }else{
            ret = EmacPsRecv(&EmacPsInstance,rx_buffers,RXBD_CNT);
            ret = EmacPsWaitRecv(&EmacPsInstance,RXBD_CNT,rx_len);
            for(int i=0; i < RXBD_CNT; i++){
                memcpy(user_addr,&rx_buffers[i],rx_len[i]);
                user_addr += rx_len[i];
                frLength[num_packet-remain+i] = rx_len[i];
            }
            remain -= RXBD_CNT;
        }
    }
    // receive packet by calling network driver's function
    // wait until you receive enough packets(`num_packet`).
    // maybe you need to call drivers' receive function multiple times ?
    return ret;
}

void do_net_send(uintptr_t addr, size_t length)
{
    // TODO:
    // send all packet
    memcpy(tx_buffer, addr, length);

    EmacPsSend(&EmacPsInstance,&tx_buffer,length); 
    EmacPsWaitSend(&EmacPsInstance);
    // maybe you need to call drivers' send function multiple times ?
}

void do_net_irq_mode(int mode)
{
    // TODO:
    // turn on/off network driver's interrupt mode
}

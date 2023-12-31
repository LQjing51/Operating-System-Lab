#ifndef INCLUDE_MAIL_BOX_
#define INCLUDE_MAIL_BOX_

#define MAX_MBOX_LENGTH (64)

// TODO: please define mailbox_t;
// mailbox_t is just an id of kernel's mail box.
#define mailbox_t int

mailbox_t mbox_open(char *);
void mbox_close(mailbox_t);
int mbox_send(mailbox_t, void *, int);
int mbox_recv(mailbox_t, void *, int);

#endif

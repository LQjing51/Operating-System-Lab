#ifndef INCLUDE_TASK3_H_
#define INCLUDE_TASK3_H_

//xtern void waiting_task1(void);
//extern void waiting_task2(void);
//extern void waited_task(void);
// [TASK2]
extern void ready_to_exit_task(void);
extern void wait_lock_task();
extern void wait_exit_task();

// [TASK3]
extern void test_condition(void);
extern void producer_task(void);
extern void consumer_task(int print_location);

extern void semaphore_add_task1(void);
extern void semaphore_add_task2(void);
extern void semaphore_add_task3(void);

extern void test_barrier(void);
extern void barrier_task(int print_location);

// [TASK4]
extern void strServer(void);
extern void strGenerator(void);

// [MULTI-CORE]
extern void test_multicore(void);
extern void test_affinity(void);


extern void bi_mailbox(void);
#endif

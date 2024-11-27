#include "mm.h"
#include "defs.h"
#include "proc.h"
#include "stdlib.h"
#include "printk.h"
extern char _stext[];
extern char _etext[];
extern char _srodata[];
extern char _erodata[];
extern char _sdata[];
extern char _edata[];
extern char _sbss[];
extern char _ebss[];
extern char _ekernel[];
extern char uapp_start[];
extern char uapp_end[];
extern void __dummy();
extern void create_mapping(uint64_t *pgtbl, uint64_t va, uint64_t pa, uint64_t sz, uint64_t perm);
struct task_struct *idle;           // idle process
struct task_struct *current;        // 指向当前运行线程的 task_struct
struct task_struct *task[NR_TASKS]; // 线程数组，所有的线程都保存在此
void setup_pgtable(struct task_struct *task) {
    task->pgd = (uint64_t *)kalloc();
    memset(task->pgd, 0, PGSIZE);
    /*******************复制内核态页表**********************/
    uint64_t tot_size = 0;
    create_mapping(task->pgd, (uint64_t)_stext,(uint64_t)_stext-PA2VA_OFFSET, 
                    _etext-_stext, XMASK|RMASK|VMASK);
    tot_size+=_etext-_stext;
    // // mapping kernel rodata -|-|R|V   只读的数据(const)
    create_mapping(task->pgd, (uint64_t)_srodata,(uint64_t)_srodata-PA2VA_OFFSET, 
                    _erodata-_srodata, RMASK|VMASK);
    tot_size+=_erodata-_srodata;
    // // mapping other memory -|W|R|V
    printk("other data len=%x %x %x\n",(uint64_t)PHY_END-(uint64_t)_sdata,(uint64_t)PHY_END,(uint64_t)_sdata);
    create_mapping(task->pgd, (uint64_t)_sdata,(uint64_t)_sdata-PA2VA_OFFSET, 
                   PHY_SIZE-tot_size, WMASK|RMASK|VMASK);
    /*****************************************************/
    //mapping UAPP
    create_mapping(task->pgd, 
                (uint64_t)uapp_start,
                (uint64_t)uapp_start-PA2VA_OFFSET, 
                uapp_end-uapp_start, 
                UMASK|XMASK|RMASK|VMASK|WMASK);
    //u-mode stack 申请一个空的页面来作为用户态栈，并映射到进程的页表中
    uint64_t ustack_end=(uint64_t)kalloc();//用户栈结束位置的虚拟地址
    create_mapping( task->pgd, 
                    USER_END-PGSIZE, 
                    (ustack_end-PA2VA_OFFSET-PGSIZE),
                    PGSIZE,
                    UMASK|RMASK|VMASK|WMASK);
}
void task_init() {
    srand(2024);

    // 1. 调用 kalloc() 为 idle 分配一个物理页

    idle= (struct task_struct *)kalloc();
    // 2. 设置 state 为 TASK_RUNNING;
    idle->state = TASK_RUNNING;
    // 3. 由于 idle 不参与调度，可以将其 counter / priority 设置为 0
    idle->counter = 0;
    idle->priority = 0;
    // 4. 设置 idle 的 pid 为 0
    idle->pid = 0;
    // 5. 将 current 和 task[0] 指向 idle
    current = idle;
    task[0] = idle;
    /* YOUR CODE HERE */
    
    // 1. 参考 idle 的设置，为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, 此外，counter 和 priority 进行如下赋值：
    //     - counter  = 0;
    //     - priority = rand() 产生的随机数（控制范围在 [PRIORITY_MIN, PRIORITY_MAX] 之间）
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 thread_struct 中的 ra 和 sp
    //     - ra 设置为 __dummy（见 4.2.2）的地址
    //     - sp 设置为该线程申请的物理页的高地址

    /* YOUR CODE HERE */
    printk("task_init start\n");
    for(int i = 1; i < NR_TASKS; i++){
        task[i] = (struct task_struct *)kalloc();
        task[i]->state = TASK_RUNNING;
        task[i]->counter = 0;
        task[i]->priority = PRIORITY_MIN + rand() % (PRIORITY_MAX - PRIORITY_MIN + 1);
        task[i]->pid = i;
        task[i]->thread.ra = (uint64_t)__dummy;
        task[i]->thread.sp = (uint64_t)task[i] + PGSIZE;
        task[i]->thread.sepc = USER_START;//将 sepc 设置为 U-Mode 的入口地址，其值为 USER_START
        task[i]->thread.sstatus = SPP_MASK|SPIE_MASK|SUM_MASK;
        task[i]->thread.sscratch = USER_END;//将 sscratch 设置为 U-Mode 的 sp，其值为 USER_END （将用户态栈放置在 user space 的最后一个页面）
        setup_pgtable(&task[i]);
    }
    printk("...task_init done!\n");
}

extern void __switch_to(struct task_struct *prev, struct task_struct *next);
void switch_to(struct task_struct *next){
    if(current == next){
        return;
    }else{
        //PRINT pid,priority,counter
        printk("switch to [PID=%d, priority=%d, counter=%d]\n", next->pid, next->priority, next->counter);
        struct task_struct *prev = current;
        current = next;
        __switch_to(prev, next);
    }
}

void do_timer() {
    if(current==idle){
        schedule();
        return;
    }
    if (current->counter > 0) {
        current->counter--;
    }
    if (current->counter == 0) {
        schedule();
    }
}

void schedule(){
    /*
    task_init 的时候随机为各个线程赋予了优先级
调度时选择 counter 最大的线程运行
如果所有线程 counter 都为 0，则令所有线程 counter = priority
即优先级越高，运行的时间越长，且越先运行
设置完后需要重新进行调度
最后通过 switch_to 切换到下一个线程
    */
    struct task_struct *next = idle;
    for(int i = 1; i < NR_TASKS; i++){
        if(task[i]->counter > next->counter){
            next = task[i];
        }
    }
    if(next->counter == 0){
        for(int i = 1; i < NR_TASKS; i++){
            task[i]->counter = task[i]->priority;
            printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n", task[i]->pid, task[i]->priority, task[i]->counter);
        }
        //重新选择
        for(int i = 1; i < NR_TASKS; i++){
            if(task[i]->counter > next->counter){
                next = task[i];
            }
        }
    }
    switch_to(next);
}
#if TEST_SCHED
#define MAX_OUTPUT ((NR_TASKS - 1) * 10)
char tasks_output[MAX_OUTPUT];
int tasks_output_index = 0;
char expected_output[] = "2222222222111111133334222222222211111113";
#include "sbi.h"
#endif

void dummy() {
    uint64_t MOD = 1000000007;
    uint64_t auto_inc_local_var = 0;
    int last_counter = -1;
    while (1) {
        if ((last_counter == -1 || current->counter != last_counter) && current->counter > 0) {
            if (current->counter == 1) {
                --(current->counter);   // forced the counter to be zero if this thread is going to be scheduled
            }                           // in case that the new counter is also 1, leading the information not printed.
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
            #if TEST_SCHED
            tasks_output[tasks_output_index++] = current->pid + '0';
            if (tasks_output_index == MAX_OUTPUT) {
                for (int i = 0; i < MAX_OUTPUT; ++i) {
                    if (tasks_output[i] != expected_output[i]) {
                        printk("\033[31mTest failed!\033[0m\n");
                        printk("\033[31m    Expected: %s\033[0m\n", expected_output);
                        printk("\033[31m    Got:      %s\033[0m\n", tasks_output);
                        sbi_system_reset(SBI_SRST_RESET_TYPE_SHUTDOWN, SBI_SRST_RESET_REASON_NONE);
                    }
                }
                printk("\033[32mTest passed!\033[0m\n");
                printk("\033[32m    Output: %s\033[0m\n", expected_output);
                sbi_system_reset(SBI_SRST_RESET_TYPE_SHUTDOWN, SBI_SRST_RESET_REASON_NONE);
            }
            #endif
        }
    }
}


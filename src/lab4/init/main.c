#include "printk.h"
#include "defs.h"
extern void test();
extern void schedule();
int start_kernel() {
    printk("2024");
    printk(" ZJU Operating System\n");
    //读取sstatus寄存器并输出
    //csr_read(0x100);
    // // printk("sstatus: %llx\n", csr_read(sstatus));
    // // printk("old sscrtach: %llx\n", csr_read(sscratch));
    // csr_write(sscratch, 0x1000);
    // printk("new sscrtach: %llx\n", csr_read(sscratch));
    schedule(); //现在更改为 OS boot 完成之后立即调度 uapp 运行
    test();
    return 0;
}

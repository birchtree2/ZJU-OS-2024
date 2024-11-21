#include "stdint.h"
#include "printk.h"
#include "clock.h"
extern void do_timer();
void trap_handler(uint64_t scause, uint64_t sepc) {
    // 通过 `scause` 判断 trap 类型
    // 如果是 interrupt 判断是否是 timer interrupt
    // 如果是 timer interrupt 则打印输出相关信息，并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.3.4 节
    // 其他 interrupt / exception 可以直接忽略，推荐打印出来供以后调试
    
    //scause见priv手册Page 96
    uint64_t mask_int=0x8000000000000000;
    if(scause&(mask_int)){//最高位为1,则是interrupt
        switch(scause&(~mask_int)){//去掉最高位
            case 0x5:
                //printk("[S] Supervisor Mode Timer Interrupt\n");
                clock_set_next_event();
                do_timer();//进行线程调度
                break;
            default:
                printk("other interrupt\n");
                break;
        }
    }
    else{//为0则是exception
        // printk("exception\n");
    }
}
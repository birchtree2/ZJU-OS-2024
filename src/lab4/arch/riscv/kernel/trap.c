#include "stdint.h"
#include "printk.h"
#include "clock.h"
#include "syscall.h"
extern void do_timer();
struct pt_regs {
    uint64_t ra;
    uint64_t norm[29]; //x3-x31 32个寄存器去掉了x0,ra,sp
    uint64_t scause;
    uint64_t sepc;
    uint64_t sp;//sp在下面 见entry.S
};
#define LOAD_PAGE_FAULT 13
#define ECALL_FROM_U_MODE 8
#define ECALL_FROM_S_MODE 9
void trap_handler(uint64_t scause, uint64_t sepc,struct pt_regs *regs) {
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
        
        if(scause==ECALL_FROM_U_MODE){
            // printk("ecall from user mode\n");
            //x17(a7)存放了系统调用号  对应norm[14]
            //x10(a0)存放了参数1  对应norm[7]
            if(regs->norm[14]==SYS_WRITE){
                regs->norm[7]=write(regs->norm[7],(const char*)regs->norm[8],regs->norm[9]);   
            }else if(regs->norm[14]==SYS_GETPID){
                regs->norm[7]=getpid();
            }
            regs->sepc+=4;//跳过ecall指令
        }else{
            printk("exception,scause=%d,sepc=%x,\n",scause,sepc);
        }
    }
}
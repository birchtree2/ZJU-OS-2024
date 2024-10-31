#include"stdint.h"
#include"sbi.h"//调用sbi_set_timer函数
// QEMU 中时钟的频率是 10MHz，也就是 1 秒钟相当于 10000000 个时钟周期
uint64_t TIMECLOCK = 1000000;

uint64_t get_cycles() {
    // 编写内联汇编，使用 rdtime 获取 time 寄存器中（也就是 mtime 寄存器）的值并返回
    uint64_t res;
    __asm__ volatile(
        "rdtime %[a0]\n"
        : [a0] "=r" (res) :: //输出到变量里面
    );
    return res;
}

void clock_set_next_event() {
    // 下一次时钟中断的时间点
    uint64_t next = get_cycles() + TIMECLOCK;
    // 使用 sbi_set_timer 来完成对下一次时钟中断的设置
    sbi_set_timer(next);
} 

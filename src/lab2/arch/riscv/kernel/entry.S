    .extern trap_handler
    .section .text.entry
    .align 2
    .globl _traps 
    .extern dummy
    .globl __dummy
    .globl __switch_to
     
_traps:
    # 1. save 32 registers and sepc to stack
    addi sp,sp,-256
    sd ra, 0(sp)
    sd gp, 8(sp) #sp最后再存
    sd tp, 16(sp)
    sd t0, 24(sp)
    sd t1, 32(sp)
    sd t2, 40(sp)
    sd s0, 48(sp)
    sd s1, 56(sp)
    sd a0, 64(sp)
    sd a1, 72(sp)
    sd a2, 80(sp)
    sd a3, 88(sp)
    sd a4, 96(sp)
    sd a5, 104(sp)
    sd a6, 112(sp)
    sd a7, 120(sp)
    sd s2, 128(sp)
    sd s3, 136(sp)
    sd s4, 144(sp)
    sd s5, 152(sp)
    sd s6, 160(sp)
    sd s7, 168(sp)
    sd s8, 176(sp)
    sd s9, 184(sp)
    sd s10, 192(sp)
    sd s11, 200(sp)
    sd t3, 208(sp)
    sd t4, 216(sp)
    sd t5, 224(sp)
    sd t6, 232(sp)
    csrr t0, sepc  #先读取sepc,再存
    sd t0, 240(sp) 
    sd sp, 248(sp) #sp最后再存
    # 2. call trap_handler
    csrr a0, scause #根据RISC-V规范,函数调用时参数传递在a0-a7中
    csrr a1, sepc
    call trap_handler
    # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
    ld ra,0(sp)
    ld gp,8(sp)
    ld tp,16(sp)
    ld t0,24(sp)
    ld t1,32(sp)
    ld t2,40(sp)
    ld s0,48(sp)
    ld s1,56(sp)
    ld a0,64(sp)
    ld a1,72(sp)
    ld a2,80(sp)
    ld a3,88(sp)
    ld a4,96(sp)
    ld a5,104(sp)
    ld a6,112(sp)
    ld a7,120(sp)
    ld s2,128(sp)
    ld s3,136(sp)
    ld s4,144(sp)
    ld s5,152(sp)
    ld s6,160(sp)
    ld s7,168(sp)
    ld s8,176(sp)
    ld s9,184(sp)
    ld s10,192(sp)
    ld s11,200(sp)
    ld t3, 208(sp)
    ld t4, 216(sp)
    ld t5, 224(sp)
    ld t6, 240(sp)
    csrw sepc, t6
    ld t6, 232(sp)
    ld sp, 248(sp)
    addi sp,sp,256
    # 4. return from trap
    sret

 __dummy:
    #把sepc设为dummy()的地址
    la t0, dummy
    csrw sepc, t0
    sret

__switch_to:
    #a0是旧的task_struct, a1是新的task_struct
    #task_struct的4个成员变量共32字节,所以从a0+32后面才是寄存器
    #保存旧的寄存器ra,sp,s0-s11
   addi t0, a0, 32 #注意结构体偏移量!!![写到报告里]
   addi t1, a1, 32
   sd ra, 0(t0)
   sd sp, 8(t0)
   sd s0, 16(t0)
   sd s1, 24(t0)
   sd s2, 32(t0)
   sd s3, 40(t0)
   sd s4, 48(t0)
   sd s5, 56(t0)
   sd s6, 64(t0)
   sd s7, 72(t0)
   sd s8, 80(t0)
   sd s9, 88(t0)
   sd s10, 96(t0)
   sd s11, 104(t0)
   #加载新的寄存器ra,sp,s0-s11
   ld ra, 0(t1)
   ld sp, 8(t1)
   ld s0, 16(t1)
   ld s1, 24(t1)
   ld s2, 32(t1)
   ld s3, 40(t1)
   ld s4, 48(t1)
   ld s5, 56(t1)
   ld s6, 64(t1)
   ld s7, 72(t1)
   ld s8, 80(t1)
   ld s9, 88(t1)
   ld s10, 96(t1)
   ld s11, 104(t1)
   #返回
   ret #用户态
 

    .extern start_kernel
    .extern sbi_set_timer
    .extern mm_init
    .extern task_init
    .section .text.init #entry->init
    .align 2
    .globl _start
    
_start:
    ##error Unimplemented
    # ------------------
    # - your code here -
    # ------------------
    la sp, boot_stack_top #la: load address of symbol to reg
    #应该在哪里call????  不能触发中断
    call mm_init #初始化内存管理
    call task_init #初始化任务管理
    # set stvec = _traps
    la t0, _traps
    csrw stvec, t0

    # set sie[STIE] = 1 
    li t0, 32 # STIE是sie中的第5位
    csrs sie, t0 #csrs是把对应位设置为1,而csrw是直接赋值

    #set first time interrupt
    rdtime a0
    li a1, 1000000
    add a0, a0, a1
    call sbi_set_timer
    
    # set sstatus[SIE] = 1
    li t0,2 # SIE是sstatus中的第1位
    csrs sstatus, t0

    
    

    j start_kernel #already declared
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size， 4KiB

    .globl boot_stack_top
boot_stack_top:

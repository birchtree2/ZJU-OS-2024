    .extern start_kernel
    .extern sbi_set_timer
    .extern mm_init
    .extern task_init
    .extern setup_vm
    .extern early_pgtbl
    .extern setup_vm_final
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
    call setup_vm #先设置虚拟内存
    call relocate #satp的设置
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

    
    call setup_vm_final
    j start_kernel #already declared

relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)

    li t0, 0xffffffdf80000000
    add ra, ra, t0
    add sp, sp, t0

    # need a fence to ensure the new translations are in use
    sfence.vma zero, zero

    # set satp with early_pgtbl
    li t2,8
    slli t2,t2,60 #satp的第63-60位是mode
    la t1, early_pgtbl #la: load address of symbol to reg
    srli t1,t1,12 #satp的第43-0位是ppn
    or t1,t1,t2
    csrw satp, t1
    ret


    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size， 4KiB

    .globl boot_stack_top
boot_stack_top:

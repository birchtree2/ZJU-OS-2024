    .extern start_kernel
    .extern sbi_set_timer
    .section .text.init #entry->init
    .align 2
    .globl _start
    
_start:
    ##error Unimplemented
    # ------------------
    # - your code here -
    # ------------------
    la sp, boot_stack_top #la: load address of symbol to reg

    # set stvec = _traps
    la t0, _traps
    csrw stvec, t0

    # set sie[STIE] = 1 
    li t0, 32 # STIE是sie中的第5位
    csrs sie, t0 #csrs是把对应位设置为1,而csrw是直接赋值

    # set first time interrupt
    rdtime a0
    li a1, 10000000
    add a0, a0, a1
    call sbi_set_timer
    # li a1, 0
    # li a2, 0
    # li a3, 0
    # li a4,0
    # li a5,0
    # li a6,0
    # li a7,0x54494d45
    # ecall
    
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

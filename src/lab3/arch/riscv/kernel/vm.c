#include"defs.h"
#include"stdlib.h"
//在vmlinux.lds中的符号：
extern char* _stext, * _etext, * _srodata, * _erodata, * _sdata, * _edata, * _sbss, * _ebss,*_skernel,*_ekernel;


const uint64_t VMASK=0x1;
const uint64_t RMASK=0x2;
const uint64_t WMASK=0x4;
const uint64_t XMASK=0x8;
/* early_pgtbl: 用于 setup_vm 进行 1GiB 的映射 */
uint64_t early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void setup_vm() {
    /* 
     * 1. 由于是进行 1GiB 的映射，这里不需要使用多级页表 
     * 2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
     *     high bit 可以忽略
     *     中间 9 bit 作为 early_pgtbl 的 index
     *     低 30 bit 作为页内偏移，这里注意到 30 = 9 + 9 + 12，即我们只使用根页表，根页表的每个 entry 都对应 1GiB 的区域
     * 3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    **/
    int index= (VM_START>>30)&((1<<9)-1);
    early_pgtbl[index]=((PHY_START>>30)&((1<<26)-1))<<28|0xf;
    index=(PHY_START>>30)&((1<<9)-1);
    early_pgtbl[index]=((PHY_START>>30)&((1<<26)-1))<<28|0xf;
    printk("setup_vm done\n");
}

/* swapper_pg_dir: kernel pagetable 根目录，在 setup_vm_final 进行映射 */
uint64_t swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

void setup_vm_final() {
    memset(swapper_pg_dir, 0x0, PGSIZE);

    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    // create_mapping(...);
    create_mapping(swapper_pg_dir, _stext,_stext-PA2VA_OFFSET, 
                    _etext-_stext, XMASK|RMASK|VMASK);
    // // mapping kernel rodata -|-|R|V   只读的数据(const)
    // create_mapping(...);
    create_mapping(swapper_pg_dir, _srodata,_srodata-PA2VA_OFFSET, 
                    _erodata-_srodata, RMASK|VMASK);
    // // mapping other memory -|W|R|V
    // create_mapping(...);
    create_mapping(swapper_pg_dir, _sdata,_sdata-PA2VA_OFFSET, 
                    _edata-_sdata, WMASK|RMASK|VMASK);
    
    create_mapping(swapper_pg_dir, _sbss,_sbss-PA2VA_OFFSET, 
                    _ebss-_sbss, WMASK|RMASK|VMASK);
    create_mapping(swapper_pg_dir, _skernel,_skernel-PA2VA_OFFSET, 
                    _ekernel-_skernel, WMASK|RMASK|VMASK);
    //map kernel code

    // set satp with swapper_pg_dir
    // asm volatile("csrw satp, %0"::"r"(((uint64_t)swapper_pg_dir>>12)|8));
    // YOUR CODE HERE

    printk("setup_vm_final done\n");

    uint64_t pgid=((uint64_t)swapper_pg_dir-PA2VA_OFFSET)/PGSIZE;
    asm volatile(
        "addi t0,%[PGID],0\n" //satp[0:44] 为页表基址,即PGID
        "li t1,8\n"
        "slli t1,t1,60\n" //把satp[63:60]  (mode) 设置为8 (sv39)
        "add t0,t0,t1\n"
        "csrw satp,t0\n"
        "sfence.vma zero, zero" // flush TLB
        :
        : [PGID] "r" (pgid)
        : "t0","t1"
    );

    // flush icache
    asm volatile("fence.i");
    return;
}


/* 创建多级页表映射关系 */
/* 不要修改该接口的参数和返回值 */
void create_mapping(uint64_t *pgtbl, uint64_t va, uint64_t pa, uint64_t sz, uint64_t perm) {
    /*
     * pgtbl 为根页表的基地址
     * va, pa 为需要映射的虚拟地址、物理地址
     * sz 为映射的大小，单位为字节
     * perm 为映射的权限（即页表项的低 8 位）
     * 
     * 创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
     * 可以使用 V bit 来判断页表项是否存在
    **/
    int k=0;
    while(sz>0){
        uint64_t cur_va=va+PGSIZE*k;
        uint64_t vpn2=(cur_va>>30)&((1<<9)-1);
        uint64_t vpn1=(cur_va>>21)&((1<<9)-1);
        uint64_t vpn0=(cur_va>>12)&((1<<9)-1);
        uint64_t *pgtb_2,*pgtb_1,*pgtb_0;//3级,2级,1级页表的基地址
        pgtb_2=pgtbl;  
        //左移10是因为低10位是标志位
        if(pgtb_2[vpn2] & VMASK) pgtb_1=(pgtb_2[vpn2]>>10)*PGSIZE+PA2VA_OFFSET;
        else{
            pgtb_1=kalloc();//kalloc()返回的是虚拟地址
            pgtb_2[vpn2]=(((uint64_t)pgtb_1-PA2VA_OFFSET)/PGSIZE)<<10|VMASK;
        }
        if(pgtb_1[vpn1] & VMASK) pgtb_0=(pgtb_1[vpn1]>>10)*PGSIZE+PA2VA_OFFSET;
        else{
            pgtb_0=kalloc();
            pgtb_1[vpn1]=(((uint64_t)pgtb_0-PA2VA_OFFSET)/PGSIZE)<<10|VMASK;
        }
        pgtb_0[vpn0]=((pa/PGSIZE)<<10)|perm; //pa/PGSIZE是最后一级页号,pa%PGSIZE是offset
        k++;
        sz-=PGSIZE;
    }
}
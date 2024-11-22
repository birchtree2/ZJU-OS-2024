#include"printk.h" //printk
#include<string.h> //memset
#include"defs.h"
#include"mm.h" //kalloc
#include"stdlib.h"
//在vmlinux.lds中的符号：
extern char _stext[];
extern char _etext[];
extern char _srodata[];
extern char _erodata[];
extern char _sdata[];
extern char _edata[];
extern char _sbss[];
extern char _ebss[];
extern char _ekernel[];


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
    early_pgtbl[VM_START >> 30 & 0x1ff] = ((PHY_START >> 30 & 0x3ffffff) << 28) + 0xf;
    early_pgtbl[PHY_START >> 30 & 0x1ff] = ((PHY_START >> 30 & 0x3ffffff) << 28) + 0xf;
    // int index= (VM_START>>30)&((1<<9)-1);
    // early_pgtbl[index]=((PHY_START>>30)&((1<<26)-1))<<28|0xf;
    // index=(PHY_START>>30)&((1<<9)-1);
    // early_pgtbl[index]=((PHY_START>>30)&((1<<26)-1))<<28|0xf;
    printk("setup_vm done\n");
}

/* swapper_pg_dir: kernel pagetable 根目录，在 setup_vm_final 进行映射 */
uint64_t swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
void create_mapping(uint64_t *pgtbl, uint64_t va, uint64_t pa, uint64_t sz, uint64_t perm);
void setup_vm_final() {
    printk("setup_vm_final start\n");
    memset(swapper_pg_dir, 0x0, PGSIZE);

    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    // create_mapping(...);
    uint64_t tot_size=0;
    printk("kernel text: %p-%p to map\n",_stext,_etext);
    create_mapping(swapper_pg_dir, (uint64_t)_stext,(uint64_t)_stext-PA2VA_OFFSET, 
                    _etext-_stext, XMASK|RMASK|VMASK);
    tot_size+=_etext-_stext;
    // // mapping kernel rodata -|-|R|V   只读的数据(const)
    // create_mapping(...);
    printk("kernel rodata: %p-%p to map\n",_srodata,_erodata);
    create_mapping(swapper_pg_dir, (uint64_t)_srodata,(uint64_t)_srodata-PA2VA_OFFSET, 
                    _erodata-_srodata, RMASK|VMASK);
    tot_size+=_erodata-_srodata;
    // // mapping other memory -|W|R|V
    // create_mapping(...);
    printk("other data len=%x %x %x\n",(uint64_t)PHY_END-(uint64_t)_sdata,(uint64_t)PHY_END,(uint64_t)_sdata);
    create_mapping(swapper_pg_dir, (uint64_t)_sdata,(uint64_t)_sdata-PA2VA_OFFSET, 
                   PHY_SIZE-tot_size, WMASK|RMASK|VMASK);

    // set satp with swapper_pg_dir
    // asm volatile("csrw satp, %0"::"r"(((uint64_t)swapper_pg_dir>>12)|8));
    // YOUR CODE HERE

    printk("setup_vm_final done\n");

    uint64_t satp=((uint64_t)swapper_pg_dir-PA2VA_OFFSET)/PGSIZE|0x8000000000000000;
    asm volatile(
        "csrw satp,%[SATP]\n"
        "sfence.vma zero, zero" // flush TLB
        :
        : [SATP] "r" (satp)
        : "t0","t1"
    );

    // flush icache
    // asm volatile("fence.i");
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
     printk("pgtbl:%p, va:%p, pa:%p, sz:%p, perm:%p\n",pgtbl,va,pa,sz,perm);
    uint64_t k=0;
    while(sz>0){
        uint64_t cur_va=va+PGSIZE*k;
        uint64_t vpn2=(cur_va>>30)&((1<<9)-1);
        uint64_t vpn1=(cur_va>>21)&((1<<9)-1);
        uint64_t vpn0=(cur_va>>12)&((1<<9)-1);
        uint64_t *pgtb_2,*pgtb_1,*pgtb_0;//3级,2级,1级页表的基地址
        pgtb_2=pgtbl;  
        //左移10是因为低10位是标志位
        // printk("sz=%d vpn2:%d, vpn1:%d, vpn0:%d\n",sz,vpn2,vpn1,vpn0);
        // printk("pgtb2[vpn2]=%p\n",pgtb_2[vpn2]);
        if(pgtb_2[vpn2] & VMASK) pgtb_1=(pgtb_2[vpn2]>>10)*PGSIZE+PA2VA_OFFSET;
        else{
            
            pgtb_1=kalloc();//kalloc()返回的是虚拟地址
            // printk("new pgtb_1:%p\n",pgtb_1);
            pgtb_2[vpn2]=(((uint64_t)pgtb_1-PA2VA_OFFSET)/PGSIZE)<<10|VMASK;
        }
        // printk("pgtb_1[vpn1]:%p %x\n",pgtb_1,pgtb_1[vpn1]);
        if(pgtb_1[vpn1] & VMASK) pgtb_0=(pgtb_1[vpn1]>>10)*PGSIZE+PA2VA_OFFSET;
        else{
            pgtb_0=kalloc();
            // printk("new pgtb_0:%p\n",pgtb_0);
            pgtb_1[vpn1]=(((uint64_t)pgtb_0-PA2VA_OFFSET)/PGSIZE)<<10|VMASK;
            // printk("new pgtb_1:%p\n",pgtb_1[vpn1]);
        }
        pgtb_0[vpn0]=((pa/PGSIZE+k)<<10)|perm; //pa/PGSIZE是最后一级页号,pa%PGSIZE是offset
        k++;
        if(sz<PGSIZE) break; //因为sz是unsigned,所以不能直接sz-=PGSIZE
        sz-=PGSIZE;
    }
    printk("map finished\n-------------------\n");
}
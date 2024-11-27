#include "syscall.h"
#include "sbi.h"
#include "proc.h"
extern struct task_struct *current;
uint64_t write(unsigned int fd, const char* buf, size_t count)
{
    for(uint64_t i=0 ; i<count ; i++)
    {
        // system call to output a char at a time
        sbi_ecall(0x1,0,buf[i],0,0,0,0,0);
    }
    return count;
}
uint64_t getpid()
{
    // simply return the value
    return current->pid;
}
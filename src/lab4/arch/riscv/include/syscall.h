#include "stdint.h"
#include "stddef.h"

#define SYS_WRITE   64
#define SYS_GETPID  172

uint64_t write(unsigned int fd, const char* buf, size_t count);
uint64_t getpid();
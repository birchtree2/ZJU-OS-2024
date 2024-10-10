C_SRC       = $(sort $(wildcard *.c)) #多个.C
OBJ=$(patsubst %.c,%.o,$(C_SRC))
 # 由于只有一个源文件printk.c，所以这里只有一个.o文件
all:$(OBJ) 
#因为上一级目录调用了${MAKE} -C lib all,所以这里要有一个all:$(OBJ)的规则

%.o:%.c # 生成.o文件的规则
	$(GCC) $(CFLAG) -c $<  
#$<表示依赖文件，这里就是printk.c
clean:
	$(shell rm *.o 2>/dev/null) 
#清除.o文件,2>/dev/null表示忽略错误信息
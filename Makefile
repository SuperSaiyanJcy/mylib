# 编译器设置
CC = gcc
CFLAGS = -std=c99 -Wall


SRCS ?= 

SRCS += ./*.c
SRCS += ${wildcard */*.c}

# 输出文件名
TARGET = main

# 编译规则
all: $(TARGET)
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) ; ./$(TARGET)

# 清理规则
clean:
	rm -f $(TARGET)
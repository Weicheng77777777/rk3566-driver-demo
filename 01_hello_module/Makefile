# Makefile
#
# 泰山派 RK3566 Linux 4.19.232 外部内核模块编译文件
#
# 当前 demo 不放进 SDK kernel 源码树中，
# 而是通过 KERNELDIR 指向 SDK 的 kernel 目录，
# 使用 Linux 内核的 Kbuild 系统编译外部模块。

# 当前模块源码所在目录
PWD := $(shell pwd)

# 泰山派 SDK 的 kernel 源码路径
# 你的 SDK 路径是：
# /home/chen20/tspi-linux-4.9/kernel
KERNELDIR ?= /home/chen20/tspi-linux-4.9/kernel

# RK3566 是 ARM64 架构
ARCH ?= arm64

# ARM64 交叉编译工具链前缀
# 如果系统安装的是 gcc-aarch64-linux-gnu，就使用这个前缀
CROSS_COMPILE ?= aarch64-linux-gnu-

# 指定要编译的内核模块
# demo.o 会被 Kbuild 编译成 demo.ko
obj-m += demo.o

# 声明伪目标，避免和同名文件冲突
.PHONY: all module clean info

# 默认目标
all: module

# 编译内核模块
#
# -C $(KERNELDIR)：
#     进入 SDK kernel 源码目录，调用内核构建系统
#
# M=$(PWD)：
#     告诉内核构建系统，外部模块源码在当前目录
#
# ARCH=$(ARCH)：
#     指定目标架构为 arm64
#
# CROSS_COMPILE=$(CROSS_COMPILE)：
#     指定交叉编译工具链前缀
#
# modules：
#     只编译外部模块
module:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

# 清理编译生成的中间文件和 demo.ko
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean

# 打印当前 Makefile 使用的关键变量，方便排查路径问题
info:
	@echo "KERNELDIR=$(KERNELDIR)"
	@echo "ARCH=$(ARCH)"
	@echo "CROSS_COMPILE=$(CROSS_COMPILE)"
	@echo "PWD=$(PWD)"

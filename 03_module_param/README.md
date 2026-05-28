
# **1_3_内核模块的传参**

这篇笔记记录我在泰山派 RK3566 上学习 Linux 内核模块传参的过程。

前面已经完成了最基础的内核模块编写、编译、加载和卸载，也验证了 `printk` 日志输出。这一节开始继续往前推进：让内核模块在加载时接收外部传入的参数。

这次实验的重点不是复杂驱动框架，而是掌握内核模块参数的基本使用方式，包括普通整型参数、数组参数和字符参数的传递，并通过 `modinfo` 和内核日志验证参数是否真正传入模块。

## **一、本节实验目标**

这节课主要学习三个内容。

第一，使用 `module_param` 给内核模块传递普通变量，例如 `int` 类型的 `led_level`。

第二，使用 `module_param_array` 给内核模块传递数组，例如 `arr=99,88,77,66,55`。

第三，使用 `MODULE_PARM_DESC` 给模块参数添加描述信息，让 `modinfo demo.ko` 能够查看参数说明。

这次我实际测试的效果是，在加载模块时通过命令行传入参数：

```bash
sudo insmod demo.ko led_level=111 arr=99,88,77,66,55 c=66
```

然后在内核日志中可以看到模块接收到的值：

```text
led_level = 111
arr[0] = 99
arr[1] = 88
arr[2] = 77
arr[3] = 66
arr[4] = 55
c = B
```

其中 `c=66` 最后打印出来是字符 `B`，因为这里传递的是 ASCII 码值，十进制 `66` 对应的字符就是 `B`。

## **二、内核模块参数的基本概念**

Linux 内核模块在加载时可以通过 `insmod` 命令传入参数。这个机制很适合用来做一些简单配置，例如调试开关、设备编号、GPIO 电平、数组配置等。

内核模块参数常用的宏有三个：

```c
module_param(name, type, perm)
module_param_array(name, type, nump, perm)
MODULE_PARM_DESC(_parm, desc)
```

`module_param` 用来传递普通变量。

`module_param_array` 用来传递数组。

`MODULE_PARM_DESC` 用来给参数添加说明信息，后面可以通过 `modinfo` 查看。

这几个宏都定义在 Linux 内核模块相关头文件中，使用前需要包含：

```c
#include <linux/init.h>
#include <linux/module.h>
```

## **三、module_param 的使用**

`module_param` 的格式如下：

```c
module_param(name, type, perm);
```

三个参数的含义分别是：

| 参数 | 含义 |
| --- | --- |
| `name` | 变量名，也是加载模块时传参使用的名字 |
| `type` | 参数类型，例如 `int`、`byte`、`charp` 等 |
| `perm` | 参数文件权限，最大一般使用 `0644` |

例如这次实验中的 `led_level`：

```c
int led_level = 0;
module_param(led_level, int, 0644);
MODULE_PARM_DESC(led_level, "type is int,level=0-1024");
```

这里定义了一个整型变量 `led_level`，默认值是 `0`。

模块加载时如果不传参，它就保持默认值。

如果加载模块时这样写：

```bash
sudo insmod demo.ko led_level=111
```

那么模块中的 `led_level` 就会变成 `111`。

## **四、module_param_array 的使用**

数组参数需要使用 `module_param_array`。

它的格式如下：

```c
module_param_array(name, type, nump, perm);
```

几个参数的含义是：

| 参数 | 含义 |
| --- | --- |
| `name` | 数组变量名 |
| `type` | 数组元素类型 |
| `nump` | 实际传入数组元素个数的指针 |
| `perm` | 参数文件权限 |

这次实验中的数组代码如下：

```c
int len = 0;
int arr[10] = {0};

module_param_array(arr, int, &len, 0644);
MODULE_PARM_DESC(arr, "type is arraylist");
```

这里定义了一个长度为 `10` 的整型数组 `arr`，同时定义了一个 `len` 用来保存实际传入的数组元素个数。

需要注意的是，`module_param_array` 的第三个参数传的是地址，所以这里写的是：

```c
&len
```

而不是：

```c
len
```

加载模块时可以这样传数组：

```bash
sudo insmod demo.ko arr=99,88,77,66,55
```

模块内部接收到数组后，`len` 的值就是 `5`，然后可以通过循环打印数组内容：

```c
for (i = 0; i < len; i++)
{
    printk("arr[%d] = %d \n", i, arr[i]);
}
```

这次实际日志中打印出了：

```text
arr[0] = 99
arr[1] = 88
arr[2] = 77
arr[3] = 66
arr[4] = 55
```

说明数组参数已经成功传入内核模块。

## **五、字符参数的传递**

这次实验里还测试了字符参数：

```c
char c = 'A';

module_param(c, byte, 0644);
MODULE_PARM_DESC(c, "type is byte");
```

这里有一个比较容易踩坑的地方。

虽然变量定义的是：

```c
char c = 'A';
```

但是在 `module_param` 里面使用的类型是：

```c
byte
```

加载模块时传递字符参数时，我这里传的是 ASCII 码值：

```bash
sudo insmod demo.ko c=66
```

最后日志打印出来是：

```text
c = B
```

因为 ASCII 码中十进制 `66` 对应的字符是 `B`。

如果直接把字符当字符串传，可能会不符合这里 `byte` 类型的解析方式。所以这次实验中，字符参数采用 ASCII 数值传递的方式更直观。

## **六、完整 demo.c 代码**

本节实验的 `demo.c` 如下：

```c
#include <linux/init.h>
#include <linux/module.h>

int led_level = 0;
module_param(led_level, int, 0644);
MODULE_PARM_DESC(led_level, "type is int,level=0-1024");

int len = 0;
int arr[10] = {0};
module_param_array(arr, int, &len, 0644);
MODULE_PARM_DESC(arr, "type is arraylist");

char c = 'A';
module_param(c, byte, 0644);
MODULE_PARM_DESC(c, "type is byte");

static int __init demo_init(void)
{
    int i = 0;

    printk("hello world %s\n", "init");
    printk("led_level = %d\n", led_level);

    for (i = 0; i < len; i++)
    {
        printk("arr[%d] = %d \n", i, arr[i]);
    }

    printk("c = %c\n", c);

    return 0;
}

static void __exit demo_exit(void)
{
    printk("hello world %s\n", "exit");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Weicheng");
MODULE_DESCRIPTION("RK3566 Linux kernel module parameter demo");
```

这段代码主要做了三件事：

第一，定义模块参数。

第二，在模块加载函数 `demo_init` 中打印传入的参数。

第三，在模块卸载函数 `demo_exit` 中打印退出信息。

## **七、Makefile**

本节仍然使用外部模块的方式编译，不直接把代码放到内核源码目录中。

目录结构可以这样安排：

```text
03_module_param/
├── demo.c
└── Makefile
```

`Makefile` 示例：

```makefile
KERNELDIR := /home/你的用户名/tspi_linux_sdk/kernel
CURRENT_PATH := $(shell pwd)

obj-m := demo.o

module:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
```

这里需要注意两点。

第一，`KERNELDIR` 要改成自己电脑上 SDK 里面真实的 `kernel` 路径。

第二，`module:` 和 `clean:` 后面的命令前面必须是 Tab，不能是普通空格。否则执行 `make` 时会报错。

## **八、编译内核模块**

进入当前实验目录：

```bash
cd 03_module_param
```

执行编译：

```bash
make
```

编译成功后，会生成：

```text
demo.ko
```

这个 `demo.ko` 就是最终要放到开发板上测试的内核模块文件。

## **九、使用 modinfo 查看模块参数**

在把模块加载到开发板之前，可以先通过 `modinfo` 查看模块信息：

```bash
sudo modinfo demo.ko
```

我这里实际看到的关键信息如下：

```text
filename:       /tmp/demo.ko
license:        GPL
name:           demo
vermagic:       4.19.232 SMP mod_unload aarch64
parm:           led_level:type is int,level=0-1024 (int)
parm:           arr:type is arraylist (array of int)
parm:           c:type is byte (byte)
```

这里重点看两部分。

第一是 `vermagic`：

```text
vermagic: 4.19.232 SMP mod_unload aarch64
```

它需要和开发板当前运行的内核版本尽量一致。之前实验中已经验证过，开发板运行的是 `4.19.232`，所以这个模块版本是匹配的。

第二是 `parm`：

```text
parm: led_level:type is int,level=0-1024 (int)
parm: arr:type is arraylist (array of int)
parm: c:type is byte (byte)
```

这说明 `MODULE_PARM_DESC` 已经生效，模块参数信息可以被 `modinfo` 正常识别出来。

这一步很重要，因为它能提前确认模块中有哪些参数可以传，不需要每次都回头看源码。

## **十、把 demo.ko 传到开发板**

编译完成后，把 `demo.ko` 传到开发板。

我这里还是延续前面实验的习惯，使用串口软件自带的 SSH/SFTP 文件传输功能，把 `demo.ko` 拖到开发板 `/root` 或 `/tmp` 目录中。

实际流程是：

```text
WSL2 中编译生成 demo.ko
↓
通过 SSH/SFTP 连接开发板
↓
打开开发板 /root 或 /tmp 目录
↓
把 demo.ko 拖进去
↓
在开发板终端中加载测试
```

如果使用命令行，也可以用 `scp`：

```bash
scp demo.ko root@开发板IP:/root/
```

## **十一、加载模块并传参**

进入开发板中 `demo.ko` 所在目录，例如：

```bash
cd /root
```

加载模块时传入参数：

```bash
sudo insmod demo.ko led_level=111 arr=99,88,77,66,55 c=66
```

这里传入了三个参数：

| 参数 | 传入值 | 作用 |
| --- | --- | --- |
| `led_level` | `111` | 测试普通整型参数 |
| `arr` | `99,88,77,66,55` | 测试数组参数 |
| `c` | `66` | 测试 byte 参数，对应 ASCII 字符 `B` |

加载之后查看内核日志：

```bash
dmesg
```

实际输出如下：

```text
[ 8163.748725] hello world init
[ 8163.748790] led_level = 111
[ 8163.748797] arr[0] = 99
[ 8163.748803] arr[1] = 88
[ 8163.748809] arr[2] = 77
[ 8163.748814] arr[3] = 66
[ 8163.748820] arr[4] = 55
[ 8163.748826] c = B
```

从日志可以确认，模块加载时传入的参数已经被内核模块正确接收。

其中 `arr` 的长度由 `len` 自动记录，所以循环只打印实际传入的 5 个元素，而不是把整个长度为 10 的数组全部打印出来。

## **十二、卸载模块**

测试完成后，卸载模块：

```bash
sudo rmmod demo
```

然后查看日志：

```bash
dmesg
```

可以看到退出信息：

```text
hello world exit
```

这说明模块的 `demo_exit` 函数也正常执行了。

## **十三、本节遇到的注意点**

这次实验中有几个细节需要特别注意。

`module_param_array` 的第三个参数必须传地址，例如：

```c
module_param_array(arr, int, &len, 0644);
```

这里不能写成 `len`，因为内核需要通过这个指针把实际传入的数组元素个数保存下来。

字符参数这部分，如果使用的是 `byte` 类型，传参时更适合传 ASCII 数值。例如：

```bash
sudo insmod demo.ko c=66
```

然后用 `%c` 打印时，就会显示：

```text
B
```

`MODULE_PARM_DESC` 不是必须的，但是非常建议加上。因为加上之后可以通过 `modinfo demo.ko` 直接看到参数说明，这对后面维护模块和调试模块都很有帮助。

权限参数这里使用的是 `0644`，表示参数文件的权限。一般测试阶段使用 `0644` 比较常见，表示所有用户可读，拥有者可写。

另外，内核模块编译时仍然要注意版本匹配。`modinfo demo.ko` 里面的 `vermagic` 要和开发板 `uname -r` 输出的内核版本一致，否则模块可能无法加载。

## **十四、本节实验结果**

本节实验最终完成了内核模块传参测试。

普通整型参数 `led_level` 可以正常传入。

数组参数 `arr` 可以正常传入，并且 `len` 能够正确记录实际数组长度。

字符参数 `c` 可以通过 ASCII 数值传入，并成功打印成字符。

`modinfo demo.ko` 能够正常显示模块参数说明。

`insmod` 加载模块、`dmesg` 查看日志、`rmmod` 卸载模块整个流程都跑通了。

通过这一节实验，我对 Linux 内核模块的参数传递机制有了更直观的理解。后面如果写实际驱动，就可以通过模块参数传入一些调试配置，例如 GPIO 编号、电平值、调试开关、设备编号等，而不需要每次都重新修改代码再编译。

---

# **泰山派 RK3566 字符设备驱动开发：从设备号原理到应用层调用**

本篇文档记录了在泰山派 RK3566 上进行**字符设备驱动**开发的进阶实战。相比之前的 Hello World 模块，本次实验深入理解了 Linux 设备号的构成原理，实现了真正的硬件抽象层接口（`file_operations`），并完成了用户空间程序对内核驱动的完整调用闭环。 [rk3566-driver-demo](https://github.com/Weicheng77777777/rk3566-driver-demo)

---

### **一、 基础理论：什么是设备号？**

在 Linux 中，内核通过**设备号**来唯一标识每一个设备。设备号本质上是一个 **32 位无符号整数**，它由两部分组成：

| 组成部分 | 占用位数 | 作用 |
| :--- | :--- | :--- |
| **主设备号 (Major)** | 高 12 位 | 代表**哪一类**设备（即由哪个驱动管理） |
| **次设备号 (Minor)** | 低 20 位 | 代表这一类中的**哪一个**具体设备 |

简单来说，主设备号决定了内核调用哪个驱动程序，而次设备号则用于区分同一驱动下的多个设备实例。理解这一点是后续 `mknod` 创建节点的基础。

---

### **二、 核心 API：驱动注册与卸载**

本课重点学习了经典的字符设备注册/注销接口。这两个函数定义在 `<linux/fs.h>` 中。

#### **1. 注册字符设备：`register_chrdev`**

```c
static inline int register_chrdev(unsigned int major, const char *name,
                                  const struct file_operations *fops)
```

*   **功能**：向内核注册一个字符设备。
*   **参数解析**：
    *   `major`（主设备号）：这是最关键的参数，它有两种工作模式。
        *   `major > 0`：**静态指定**设备号。需要开发者自己确保该设备号未被系统占用，否则注册失败。
        *   `major = 0`：**系统自动分配**设备号。这是推荐做法，可以避免设备号冲突。
    *   `name`：设备名字（显示在 `/proc/devices` 中）。
    *   `fops`：文件操作结构体指针，是驱动的核心。
*   **返回值（注意区分两种情况）**：
    *   当 `major > 0`（静态）：成功返回 `0`，失败返回错误码。
    *   当 `major = 0`（动态）：**成功返回系统分配的设备号**，失败返回错误码。

> 正因为动态模式下成功返回的是设备号本身，所以代码中通常用一个变量 `major` 去接收返回值，再用它来创建节点。 [demo.md](demo.md)

#### **2. 注销字符设备：`unregister_chrdev`**

```c
static inline void unregister_chrdev(unsigned int major, const char *name)
```

*   **功能**：注销字符设备，释放占用的主设备号。
*   **参数**：`major`（主设备号）、`name`（设备名）。
*   **返回值**：无。

务必在模块的 `__exit` 退出函数中调用它，否则会导致设备号泄漏。

---

### **三、 关键桥梁：`file_operations` 结构体**

`file_operations` 是连接“用户态系统调用”与“内核态驱动函数”的桥梁。结构体中各成员的函数原型如下，编写驱动函数时签名必须与之严格对应： [笔记.txt](笔记.txt)

```c
int     (*open)    (struct inode *, struct file *);
ssize_t (*read)    (struct file *, char __user *, size_t, loff_t *);
ssize_t (*write)   (struct file *, const char __user *, size_t, loff_t *);
int     (*release) (struct inode *, struct file *);
```

在本次实验中，我们将自定义函数挂载到该结构体上：

```c
struct file_operations fops = {
    .open    = my_open,
    .read    = my_read,
    .write   = my_write,
    .release = my_close   // 注意：用户态的 close() 对应内核态的 release
};
```

此外，驱动还需通过宏声明作者与许可证信息，否则加载时可能出现 `taints kernel` 之外的警告：

```c
MODULE_LICENSE("GPL");
MODULE_AUTHOR("WC WC@gmail.com");
```

---

### **四、 应用层开发：如何包含头文件？**

为了在 C 程序中调用驱动，需要使用 Linux 系统调用。通过 `man` 手册可以准确查到每个系统调用所需的头文件。命令中的数字 `2` 专指**系统调用（System Calls）**这一手册章节：

| 查询命令 | 对应系统调用 | 需要包含的头文件 |
| :--- | :--- | :--- |
| `man 2 open` | `open()` | `<sys/types.h>` `<sys/stat.h>` `<fcntl.h>` |
| `man 2 read` | `read()` | `<unistd.h>` |
| `man 2 write` | `write()` | `<unistd.h>` |
| `man 2 close` | `close()` | `<unistd.h>` |

因此，一个完整的测试程序通常以如下头文件开头：

```c
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
```

---

### **五、 实战操作流程与日志验证**

完整的调试链路如下： [MobaXterm_192.168.88.20root_20260608_142120.txt](MobaXterm_192.168.88.20root_20260608_142120.txt)

#### **1. 加载驱动并查看设备号**
```bash
insmod demo.ko
# 查看系统分配的主设备号
cat /proc/devices | grep chardev
# 结果显示为：236 chardev
```

#### **2. 创建设备节点**
Linux 中“一切皆文件”，驱动加载后需要手动创建设备文件供应用访问。这里的主设备号 `236` 必须与上一步查到的一致：
```bash
# mknod /dev/设备名 c 主设备号 次设备号
sudo mknod /dev/chardev c 236 0
# 修改节点权限，否则普通用户无法读写
sudo chmod 777 /dev/chardev
```

#### **3. 运行测试程序**
编译并运行 C 测试程序，终端输出：
```text
open success
read success
write success
close success
```

#### **4. 内核日志验证（关键步骤）**
使用 `dmesg` 查看内核是否真正收到了请求，通过特征字符 `WC` 定位：
```bash
dmesg | tail -n 20
```
内核反馈结果：
```text
[ 4270.262677] open!WC
[ 4270.262821] read!WC
[ 4270.262833] write!WC
[ 4270.262918] close!WC
```
这标志着从用户态到内核态的调用路径完全打通。

---

### **六、 学习总结**

*   **设备号原理**：理解了 32 位设备号 = 高 12 位主设备号 + 低 20 位次设备号，主设备号定位驱动，次设备号定位实例。
*   **注册机制**：掌握了 `register_chrdev` 静态（`major>0`）与动态（`major=0`）两种分配方式及其不同的返回值语义。
*   **节点创建**：明白 `insmod` 只加载了逻辑，还需 `mknod` 创建物理访问入口，并通过 `chmod` 赋权。
*   **调用链路**：`APP (open) → VFS → Driver (my_open) → Hardware`。
*   **调试工具**：熟练运用 `man 2` 查头文件、`dmesg | tail` 看内核打印。

---

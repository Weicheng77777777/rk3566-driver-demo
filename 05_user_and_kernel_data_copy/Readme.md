### **泰山派 RK3566 驱动开发实战：用户空间与内核空间数据拷贝**

本篇 README 记录了在泰山派 RK3566 上实现字符设备驱动“进阶版”的过程。重点在于打通**应用层（User Space）**与**内核层（Kernel Space）**的数据壁垒，实现数据的双向传递。 [rk3566-driver-demo](https://github.com/Weicheng77777777/rk3566-driver-demo)

---

### **一、 理论基础：Linux 系统分层与内存隔离**

根据实验笔记，Linux 系统在逻辑上被划分为不同的层级，且地址空间是严格隔离的： [笔记.txt](笔记.txt)

| 层级 | 空间划分 | 虚拟内存范围 | 核心职能 |
| :--- | :--- | :--- | :--- |
| **应用层 (App)** | 用户空间 | 0 - 3G | 运行逻辑代码，通过系统调用（open/read/write）请求服务。 |
| **内核层 (Kernel)** | 内核空间 | 3 - 4G | 向上提供接口，向下控制硬件。驱动程序运行于此。 |
| **硬件层 (HAL)** | 物理硬件 | - | LED、LCD、各种传感器等物理实体。 |

**核心挑战**：用户空间与内核空间的内存不能直接互访。内核提供了一套专门的 API 来执行跨空间的数据搬运。

---

### **二、 核心 API：数据的“摆渡人”**

驱动程序中使用以下两个函数完成数据拷贝： [笔记.txt](李sir笔记.txt)

#### **1. 从内核拷贝到用户：`copy_to_user`**
*   **功能**：读取硬件/内核缓存数据，发送给应用程序。
*   **函数原型**：`static inline int copy_to_user(void __user *to, const void *from, unsigned long n)`
*   **参数**：`to`（用户地址）、`from`（内核地址）、`n`（字节数）。
*   **返回值**：成功返回 `0`，失败返回未拷贝的字节数。

#### **2. 从用户拷贝到内核：`copy_from_user`**
*   **功能**：接收应用程序发送的数据，用于控制硬件或更新缓存。
*   **函数原型**：`static inline int copy_from_user(void *to, const void __user *from, unsigned long n)`
*   **参数**：`to`（内核地址）、`from`（用户地址）、`n`（字节数）。
*   **返回值**：成功返回 `0`，失败返回未拷贝的字节数。

---

### **三、 驱动程序设计：`demo.c`**

我们在驱动中定义了一个内核缓冲区 `kbuf[128]`。通过 `my_read` 和 `my_write` 函数实现数据的存取。

```c
#include <linux/uaccess.h> // 必须包含该头文件

char kbuf[128] = {0};

ssize_t my_read (struct file *file, char __user *ubuf, size_t size, loff_t *offset)
{
    if(size > sizeof(kbuf)) size = sizeof(kbuf);
    // 将内核 kbuf 拷贝给用户 ubuf
    if(copy_to_user(ubuf, kbuf, size)) {
        printk("copy data to user fail!
");
        return -EIO;
    }
    return size;
}

ssize_t my_write (struct file *file, const char __user *ubuf, size_t size, loff_t *offset)
{
    if(size > sizeof(kbuf)) size = sizeof(kbuf);
    // 将用户 ubuf 拷贝到内核 kbuf
    if(copy_from_user(kbuf, ubuf, size)) {
        printk("copy data form user fail!
");
        return -EIO;
    }
    return size;
}
```

---

### **四、 测试程序设计：`test.c`**

应用层通过标准文件操作验证数据传递：
1. `write`: 将字符串 `"123456"` 写入内核。
2. `read`: 从内核将刚才写入的内容读回到另一个缓冲区 `ubuf`。

```c
char buf[32] = "123456";
char ubuf[64] = {0};
int fd = open("/dev/chardev", O_RDWR);
// ...
    // 1. 准备要发送的数据
    memcpy(ubuf, buf, sizeof(buf));
    printf("准备写入驱动的数据: %s\n", ubuf);
    // 2. 写入驱动
    write(fd, ubuf, sizeof(ubuf));
    // 3. 【重要】清空缓冲区！确保接下来的 read 不是在读旧数据
    memset(ubuf, 0, sizeof(ubuf));
    printf("清空缓冲区后，ubuf的内容为: [%s] (应该是空的)\n", ubuf);
    // 4. 从驱动读回
    read(fd, ubuf, sizeof(ubuf));
    printf("从驱动读回的数据: %s\n", ubuf);
```

---

### **五、 实战操作与日志验证**

根据串口实测，完整的环境搭建与验证过程如下：

#### **1. 加载驱动与确认号**
```bash
insmod demo.ko
cat /proc/devices | grep chardev  # 查到主设备号 236
```

#### **2. 创建节点与赋权**
*注意：mknod 必须指定类型 c 和设备号，否则会像日志中第一次尝试那样报错：`missing operand`。*
```bash
# sudo mknod /dev/设备名 c 主设备号 次设备号
sudo mknod /dev/chardev c 236 0
sudo chmod 777 /dev/chardev
```

#### **3. 执行测试与结果分析**
运行测试程序后，成功观察到数据从内核“折返”回应用层：
```bash
root@tspi-ubuntu22:~# ./test
ubuf = 123456 
```
或者
```bash
root@tspi-ubuntu22:~# ./test
准备写入驱动的数据: 123456
清空缓冲区后，ubuf的内容为: [] (应该是空的)
从驱动读回的数据: 123456
root@tspi-ubuntu22:~#
```

这证明了 `copy_from_user` 成功将应用层的 `"123456"` 存入内核，而 `copy_to_user` 随后将其原样取回。

---

### **六、 学习总结**

1.  **分层隔离**：理解了 Linux 用户态与内核态的地址空间隔离（0-3G 与 3-4G），确认了通过普通指针无法跨空间访问。
2.  **安全传输**：掌握了专用拷贝 API 的用法及头文件 `<linux/uaccess.h>` 的引用。
3.  **调试闭环**：通过 `memcpy` -> `write` -> `read` -> `printf` 形成了一个完整的逻辑闭环，验证了驱动程序的数据处理能力。
4.  **操作规范**：熟练了 `mknod` 的完整语法，避免了因参数缺失导致的创建失败。
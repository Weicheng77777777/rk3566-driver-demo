下面我直接按你**实际开发流程**重写一版 `README.md`，不沿用之前那种“标准模板式”写法，而是贴合你现在真实在做的事情：

- 已经会烧写镜像
- 先配串口
- 再配 Wi‑Fi
- 再配 SSH
- 脚本先在本地 VS Code 写
- 通过串口软件的 SSH/SFTP 功能拖到开发板 `/root`
- 先手动测试
- 没问题再放到 `/etc/init.d/`
- 然后在 WSL2 里编译内核模块
- 最后上传 `.ko` 到板子上 `insmod/rmmod`

我也会说明我改了什么、为什么这么改。

---

```markdown
# 泰山派 RK3566 Linux 内核模块开发环境搭建与 Hello Demo 实战

这篇文档记录我在泰山派 RK3566 上搭建 Linux 内核模块开发环境，并完成第一个最小内核模块 `demo.ko` 编写、编译、加载和卸载的完整过程。

这不是一篇纯概念说明，而是一篇贴合实际开发流程的实操记录。

本文默认你已经学会了系统镜像烧写，所以不再展开烧写步骤。本文重点从**开发环境配置**开始，包括：

- 串口连接
- Wi‑Fi 配置
- SSH 连接
- 开机自动联网脚本测试与固化
- WSL2 下准备 SDK 和交叉编译环境
- 编写并编译最小内核模块
- 上传到开发板并验证 `insmod/rmmod`

---

## 一、本文使用的文件

本次使用的是泰山派网盘中的两个文件：

- `kernel4.9` 的 Buildroot 系统镜像  
  对应：`第06章.【立创·泰山派】系统镜像`
- `tspi_linux_sdk_repo_20240131.tar.gz`  
  对应：`第05章.【立创·泰山派】系统SDK`

虽然镜像目录名里写的是 `kernel4.9`，但实际运行和编译时必须看的不是文件夹名字，而是**当前板子运行内核版本**和 **SDK 对应 kernel 源码版本**。

这一点非常重要。

---

## 二、先确认：内核版本必须一致

在做 Linux 内核模块开发时，最重要的一件事不是先写代码，而是先确认：

> 编译 `.ko` 使用的内核源码版本，要和开发板当前运行的内核版本一致。

否则后续即使代码写对了，模块也可能加载失败，报错通常类似：

```bash
Invalid module format
```

先在开发板上通过串口登录系统，执行：

```bash
uname -r
```

我的开发板输出是：

```bash
4.19.232
```

然后在 WSL2 中进入 SDK 的 `kernel` 目录，查看内核版本：

```bash
cd ~/tspi-linux-4.9/kernel
grep -E "^(VERSION|PATCHLEVEL|SUBLEVEL|EXTRAVERSION)" Makefile
```

输出如下：

```bash
VERSION = 4
PATCHLEVEL = 19
SUBLEVEL = 232
EXTRAVERSION =
```

说明 SDK 的 kernel 源码版本也是：

```text
4.19.232
```

后面模块编译完成后，再用下面的命令确认：

```bash
modinfo demo.ko
```

重点看这一项：

```bash
vermagic: 4.19.232 SMP mod_unload aarch64
```

判断是否能正常加载模块时，我实际就是看这三处是否对上：

- 开发板 `uname -r`
- SDK `kernel/Makefile` 版本
- `modinfo demo.ko` 里的 `vermagic`

三者尽量一致，后面才值得继续做。

---

## 三、串口连接开发板

镜像烧写完成后，先通过串口连接开发板。

我这里使用的串口参数如下：

```text
波特率：1500000
数据位：8
停止位：1
校验位：None
流控：None
```

串口连通之后，先不要急着写驱动。我的实际流程是先把网络和远程开发链路打通，因为后面频繁调试 `.ko` 文件时，如果每次都只靠串口输入命令，会比较低效。

所以我接下来的流程是：

```text
串口登录
    ↓
配置 Wi‑Fi
    ↓
确认开发板拿到 IP
    ↓
启动 SSH
    ↓
电脑连接 SSH
    ↓
本地写脚本和代码
    ↓
传到开发板测试
```

---

## 四、先手动配置 Wi‑Fi

这个 Buildroot 系统比较精简，没有 `nmcli`，所以不能用桌面 Linux 常见的 NetworkManager 命令。

先确认开发板上有没有这些工具：

```bash
which wpa_supplicant
which udhcpc
which iwconfig
which iwlist
```

我这里确认都有：

```bash
/usr/sbin/wpa_supplicant
/sbin/udhcpc
/sbin/iwconfig
/sbin/iwlist
```

电脑端先开热点。我这里实际测试时，感觉这个 Wi‑Fi 更适合连接 **2.4G 热点**。如果后续有大文件传输需求，还是更建议走网线直连，但这篇文档先按 Wi‑Fi + SSH 的开发方式来讲。

先在开发板上创建 Wi‑Fi 配置文件：

```bash
mkdir -p /etc/wifi

cat > /etc/wifi/wpa_supplicant.conf <<'EOF'
ctrl_interface=/var/run/wpa_supplicant
update_config=1

network={
    ssid="你的热点名称"
    psk="你的热点密码"
    key_mgmt=WPA-PSK
}
EOF
```

然后手动执行连接流程：

```bash
ip addr flush dev wlan0
ip link set wlan0 down
sleep 1
ip link set wlan0 up

killall wpa_supplicant 2>/dev/null
killall udhcpc 2>/dev/null

mkdir -p /var/run/wpa_supplicant

wpa_supplicant -B -i wlan0 -Dnl80211 -c /etc/wifi/wpa_supplicant.conf
sleep 5
udhcpc -i wlan0
```

如果 `-Dnl80211` 不行，可以换成：

```bash
wpa_supplicant -B -i wlan0 -Dwext -c /etc/wifi/wpa_supplicant.conf
```

连接后查看状态：

```bash
iwconfig wlan0
ip addr show wlan0
```

如果看到了类似：

```text
ESSID:"你的热点名称"
inet 192.168.x.x/24
```

说明 Wi‑Fi 已经连上，开发板也拿到了 IP。

---

## 五、确认 SSH 可用

Wi‑Fi 联网之后，下一步就是把 SSH 打通。

先在开发板上查看 22 端口是否监听：

```bash
netstat -lntp | grep :22
```

如果没有，就启动 dropbear：

```bash
/etc/init.d/S50dropbear start
```

然后在电脑上测试 SSH 登录：

```bash
ssh root@开发板IP
```

SSH 连通后，后面很多工作就不需要依赖串口输入长命令了。我的实际习惯是：

- 串口主要负责第一次联网配置和开机验证
- SSH 负责后续日常登录
- 文件传输主要通过串口软件的 SSH/SFTP 文件管理拖拽完成

---

## 六、Wi‑Fi 脚本先在本地写，再放到开发板 `/root` 测试

这里是我实际使用的流程。

我不是一开始就直接在开发板的 `/etc/init.d/` 里面写脚本，而是先在本地 VS Code 里写好脚本文件，再通过串口软件的 SSH/SFTP 文件传输功能，把脚本拖到开发板 `/root` 目录，手动测试成功之后，才复制到 `/etc/init.d/`。

这个流程更稳，原因很简单：

> 开机脚本如果一开始就写错，后面每次开机都执行错误逻辑，排查会更麻烦。  
> 先在 `/root` 测试通过，再放到 `/etc/init.d/`，更安全也更符合实际开发习惯。

### 1. 在本地新建脚本 `wifi_sta`

我在本地 VS Code 中创建文件：

```bash
wifi_sta
```

内容如下：

```bash
#!/bin/sh

echo "Starting WiFi STA..."

ip addr flush dev wlan0
ip link set wlan0 down
sleep 1
ip link set wlan0 up

killall wpa_supplicant 2>/dev/null
killall udhcpc 2>/dev/null

mkdir -p /var/run/wpa_supplicant

wpa_supplicant -B -i wlan0 -Dnl80211 -c /etc/wifi/wpa_supplicant.conf
sleep 5

udhcpc -i wlan0

echo "WiFi STA started."
```

如果你板子上 `nl80211` 不工作，把这一行改成：

```bash
wpa_supplicant -B -i wlan0 -Dwext -c /etc/wifi/wpa_supplicant.conf
```

### 2. 通过串口软件的 SSH/SFTP 功能拖到开发板 `/root`

在开发板已经联网并且 SSH 可用后，可以直接使用串口软件的 SSH/SFTP 文件传输功能连接开发板。

我的实际操作方式是：

```text
电脑本地写好 wifi_sta
    ↓
串口软件通过 SSH/SFTP 连接开发板
    ↓
打开开发板 /root 目录
    ↓
把 wifi_sta 直接拖进去
```

这样传输完成后，不需要再单独写 `scp` 命令。

然后进入开发板终端，确认文件已经存在：

```bash
cd /root
ls
```

如果看到：

```bash
wifi_sta
```

说明传输成功。

### 3. 添加执行权限并手动测试

文件拖过去之后，我实际做的下一步就是直接加权限：

```bash
chmod +x /root/wifi_sta
```

然后执行：

```bash
/root/wifi_sta
```

测试后查看 Wi‑Fi 状态：

```bash
iwconfig wlan0
ip addr show wlan0
```

如果看到：

```text
ESSID:"你的热点名称"
inet 192.168.x.x/24
```

说明脚本本身没有问题。

再确认 SSH 是否正常：

```bash
netstat -lntp | grep :22
```

如果还没起来，可以手动执行：

```bash
/etc/init.d/S50dropbear start
```

然后回到电脑侧测试：

```bash
ssh root@开发板IP
```

如果可以正常登录，说明这一套联网脚本已经通过测试。

### 4. 测试成功后再复制到 `/etc/init.d/`

确认 `/root/wifi_sta` 测试成功后，再复制为开机脚本：

```bash
cp /root/wifi_sta /etc/init.d/S42wifi_sta
chmod +x /etc/init.d/S42wifi_sta
```

这里我保留了测试阶段和开机阶段两个不同名字：

- 测试文件：`/root/wifi_sta`
- 开机脚本：`/etc/init.d/S42wifi_sta`

这样做更清晰，也更严谨。

### 5. 重启验证

最后重启开发板：

```bash
reboot
```

重启后先通过串口查看：

```bash
ip addr show wlan0
netstat -lntp | grep :22
```

如果已经自动联网，并且 SSH 服务正常，再在电脑端执行：

```bash
ssh root@开发板IP
```

如果可以登录，说明网络开发环境已经配置完成。

---

## 七、在 WSL2 Ubuntu 中准备 SDK 编译环境

网络链路稳定之后，我再开始在 WSL2 Ubuntu 中准备交叉编译环境。

先安装工具链：

```bash
sudo apt update
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu make -y
```

然后解压 SDK：

```bash
mkdir -p ~/tspi-linux-4.9
cd ~/tspi-linux-4.9
tar -xzf tspi_linux_sdk_repo_20240131.tar.gz
```

进入 `kernel` 目录确认版本：

```bash
cd ~/tspi-linux-4.9/kernel
grep -E "^(VERSION|PATCHLEVEL|SUBLEVEL|EXTRAVERSION)" Makefile
```

如果你在编译外部模块时遇到下面这个报错：

```bash
ERROR: Kernel configuration is invalid.
include/generated/autoconf.h or include/config/auto.conf are missing.
Run 'make oldconfig && make prepare' on kernel src to fix it.
```

这说明内核源码树还没有准备好，不是 `demo.c` 写错了，而是 kernel 源码树还缺少外部模块编译所需的配置文件。

外部模块编译至少依赖这些文件：

```text
.config
include/config/auto.conf
include/generated/autoconf.h
```

所以这里要先按 SDK 的编译流程准备好 kernel 源码树，再继续编译外部模块。

---

## 八、创建独立的外部模块目录

我没有把 `demo.c` 直接放在 SDK 的 `kernel` 目录中，而是单独建了一个外部模块工程目录。

这样做的好处是：

- 源码和 SDK 分离
- 编译产物不污染内核源码树
- 后续增加新 demo 时结构更清晰

创建目录：

```bash
mkdir -p ~/rk3566-driver-demo/01_hello_module
cd ~/rk3566-driver-demo/01_hello_module
```

目录结构如下：

```text
01_hello_module/
├── demo.c
└── Makefile
```

---

## 九、编写第一个内核模块 `demo.c`

在 `01_hello_module` 目录中创建 `demo.c`：

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int __init demo_init(void)
{
    pr_info("demo: module init, hello Taishan RK3566\n");
    return 0;
}

static void __exit demo_exit(void)
{
    pr_info("demo: module exit, goodbye Taishan RK3566\n");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Taishan RK3566 Demo");
MODULE_DESCRIPTION("A simple Linux kernel module demo for Taishan RK3566");
MODULE_VERSION("1.0");
```

这个 demo 很简单，只做两件事：

- `insmod demo.ko` 时打印加载日志
- `rmmod demo` 时打印卸载日志

这是最适合验证整个开发链路是否打通的第一个实验。

---

## 十、编写外部模块 Makefile

在同目录创建 `Makefile`：

```makefile
PWD := $(shell pwd)

KERNELDIR ?= /home/chen20/tspi-linux-4.9/kernel

ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-

obj-m += demo.o

.PHONY: all module clean info

all: module

module:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean

info:
	@echo "KERNELDIR=$(KERNELDIR)"
	@echo "ARCH=$(ARCH)"
	@echo "CROSS_COMPILE=$(CROSS_COMPILE)"
	@echo "PWD=$(PWD)"
```

这里有两个注意点。

第一，`KERNELDIR` 要改成你自己的 SDK kernel 路径。

第二，`module:` 和 `clean:` 后面的命令前面必须是 **Tab**，不能是空格，否则 `make` 会报错。

---

## 十一、编译内核模块

在 `01_hello_module` 目录中执行：

```bash
make
```

成功后会生成：

```text
demo.ko
```

然后检查模块信息：

```bash
modinfo demo.ko
```

重点确认：

```bash
vermagic: 4.19.232 SMP mod_unload aarch64
```

如果这里和开发板 `uname -r` 一致，就说明这个模块基本已经编对了。

---

## 十二、把 `demo.ko` 放到开发板 `/root` 测试

和前面的 `wifi_sta` 一样，我实际更习惯用串口软件的 SSH/SFTP 文件传输功能来拖文件，而不是每次手写 `scp`。

实际流程就是：

```text
本地 WSL2 编译出 demo.ko
    ↓
串口软件通过 SSH/SFTP 连接开发板
    ↓
打开开发板 /root
    ↓
把 demo.ko 拖进去
```

传输完成后，在开发板上进入 `/root`：

```bash
cd /root
ls
```

确认 `demo.ko` 已经在里面。

当然，如果你更习惯命令行，也可以使用：

```bash
scp demo.ko root@开发板IP:/root/
```

但就我的实际流程来说，拖拽传输更直观。

---

## 十三、加载和卸载模块

在开发板 `/root` 目录执行：

```bash
insmod demo.ko
dmesg | tail -n 20
```

如果看到类似输出：

```text
demo: loading out-of-tree module taints kernel.
demo: module init, hello Taishan RK3566
```

说明模块加载成功。

其中：

```text
loading out-of-tree module taints kernel
```

表示这是一个外部编译的内核模块，这个提示是正常的，不代表失败。

接着执行卸载：

```bash
rmmod demo
dmesg | tail -n 20
```

如果看到：

```text
demo: module exit, goodbye Taishan RK3566
```

说明卸载也正常。

我这次实际测试时，最终日志就是：

```text
demo: loading out-of-tree module taints kernel.
demo: module init, hello Taishan RK3566
demo: module exit, goodbye Taishan RK3566
```

这说明整个开发链路已经跑通了。

---

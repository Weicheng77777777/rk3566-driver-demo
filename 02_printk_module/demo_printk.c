/*
 * demo_printk.c
 *
 * 泰山派 RK3566 Linux 内核模块示例：
 * 演示 printk 的基本使用方法，以及常见日志等级。
 *
 * 学习目标：
 * 1. 理解 printk 是什么
 * 2. 理解 KERN_ERR / KERN_WARNING / KERN_INFO 等日志等级
 * 3. 学会通过 dmesg 查看内核日志
 * 4. 学会通过 /proc/sys/kernel/printk 查看当前控制台日志等级
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

/*
 * 模块加载函数
 *
 * 执行 insmod demo_printk.ko 时，会调用这里。
 * 这里故意打印多种日志等级，方便观察效果。
 */
static int __init demo_init(void)
{
    printk(KERN_ERR     "demo_printk: [KERN_ERR] hello world %s\n", "init");
    printk(KERN_WARNING "demo_printk: [KERN_WARNING] warning level message\n");
    printk(KERN_INFO    "demo_printk: [KERN_INFO] info level message\n");

    return 0;
}

/*
 * 模块卸载函数
 *
 * 执行 rmmod demo_printk 时，会调用这里。
 */
static void __exit demo_exit(void)
{
    printk(KERN_INFO "demo_printk: module exit\n");
}

/* 注册模块加载入口函数 */
module_init(demo_init);

/* 注册模块卸载出口函数 */
module_exit(demo_exit);

/* 模块许可证 */
MODULE_LICENSE("GPL");

/* 作者信息 */
MODULE_AUTHOR("Weicheng");

/* 模块描述 */
MODULE_DESCRIPTION("A simple printk loglevel demo for Taishan RK3566");

/* 模块版本 */
MODULE_VERSION("1.0");

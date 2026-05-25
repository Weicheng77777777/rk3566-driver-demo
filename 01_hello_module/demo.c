/*
 * demo.c
 *
 * 泰山派 RK3566 Linux 4.19.232 内核模块 demo
 *
 * 这个 demo 演示最基础的 Linux 内核模块结构：
 * 1. 模块加载入口函数 demo_init
 * 2. 模块卸载出口函数 demo_exit
 * 3. module_init / module_exit 注册入口和出口
 * 4. MODULE_LICENSE 等模块信息
 */

#include <linux/init.h>      /* 提供 __init、__exit 等宏 */
#include <linux/module.h>    /* 提供 module_init、module_exit、MODULE_LICENSE 等 */
#include <linux/kernel.h>    /* 提供 pr_info 等内核日志接口 */

/*
 * 模块加载函数
 *
 * 当在板子上执行：
 *     insmod demo.ko
 *
 * 内核会调用这个函数。
 *
 * 返回值：
 *     0 表示模块加载成功
 *     非 0 表示模块加载失败
 */
static int __init demo_init(void)
{
    pr_info("demo: module init, hello Taishan RK3566\n");

    return 0;
}

/*
 * 模块卸载函数
 *
 * 当在板子上执行：
 *     rmmod demo
 *
 * 内核会调用这个函数。
 *
 * 这里主要做资源释放。
 * 当前 demo 没有申请 GPIO、中断、内存、字符设备等资源，
 * 所以这里只打印一条日志。
 */
static void __exit demo_exit(void)
{
    pr_info("demo: module exit, goodbye Taishan RK3566\n");
}

/*
 * 注册模块加载入口函数
 */
module_init(demo_init);

/*
 * 注册模块卸载出口函数
 */
module_exit(demo_exit);

/*
 * 模块许可证
 *
 * 建议写 GPL。
 * 如果不写，加载模块时内核可能提示 license 缺失，
 * 并且某些 GPL-only 符号不能使用。
 */
MODULE_LICENSE("GPL");

/*
 * 模块作者信息
 */
MODULE_AUTHOR("Taishan RK3566 Demo By Weicheng");

/*
 * 模块描述信息
 */
MODULE_DESCRIPTION("A simple Linux kernel module demo for Taishan RK3566");

/*
 * 模块版本
 */
MODULE_VERSION("1.0");

#include <linux/init.h>    // 内核模块的初始化/退出函数相关头文件
#include <linux/module.h>  // 内核模块的加载/卸载、参数传递等核心头文件

/*
 * 内核模块参数（module_param）允许在 insmod/modprobe 加载模块时
 * 通过命令行传递参数值，无需重新编译模块
 */

// 整型参数：led_level，取值范围 0-1024（PWM 占空比常用范围）
int led_level = 0;
module_param(led_level, int, 0644);                  // 参数名, 类型, 文件权限（/sys/module/.../parameters/ 下可见）
MODULE_PARM_DESC(led_level, "type is int,level=0-1024");  // 参数描述

// 数组参数：可接收多个整型值，len 自动记录实际传入的元素个数
int len = 0;
int arr[10] = {0};
module_param_array(arr, int, &len, 0644);            // &len 会被自动填充为用户传入的元素数量
MODULE_PARM_DESC(arr, "type is arraylist");

// 字节类型参数：用 char 存储，实际只取一个字符
char c = 'A';
module_param(c, byte, 0644);                         // byte 类型对应单个字符
MODULE_PARM_DESC(c, "type is byte");

/*
 * 模块入口函数：insmod 加载模块时自动调用
 * __init 标记表示该函数在初始化完成后会被释放，节省内存
 */
static int __init demo_init(void)
{
    int i = 0;

    printk("hello world %s\n", "init");              // 打印到内核日志（dmesg 可查看）
    printk("led_level = %d\n", led_level);

    // 打印数组参数的每个元素
    for (i = 0; i < len; i++)
    {
        printk("arr[%d] = %d \n", i, arr[i]);
    }

    printk("c = %c\n", c);

    return 0;                                        // 返回 0 表示加载成功
}

/*
 * 模块出口函数：rmmod 卸载模块时自动调用
 * __exit 标记表示该函数在模块内建（非模块化）时会被丢弃
 */
static void __exit demo_exit(void)
{
    printk("hello world %s\n", "exit");
}

// 注册模块的入口和出口函数
module_init(demo_init);
module_exit(demo_exit);

// 模块元信息：许可证、作者、描述
MODULE_LICENSE("GPL");                                // 必须声明，否则内核会认为模块污染了内核
MODULE_AUTHOR("Weicheng");
MODULE_DESCRIPTION("RK3566 Linux kernel module parameter demo");  // 在 RK3566 平台上演示模块参数用法

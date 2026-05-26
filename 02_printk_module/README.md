# 02_printk_module

日志等级说明
Linux 内核日志等级从 0 到 7，数字越小，优先级越高。
常见等级如下：
| 日志宏 | 数值 | 含义 |
| --- | --- | --- |
| KERN_EMERG | 0 | 系统不可用 |
| KERN_ALERT | 1 | 必须立即处理 |
| KERN_CRIT | 2 | 严重错误 |
| KERN_ERR | 3 | 错误信息 |
| KERN_WARNING | 4 | 警告信息 |
| KERN_NOTICE | 5 | 普通但重要的信息 |
| KERN_INFO | 6 | 普通信息 |
| KERN_DEBUG | 7 | 调试信息 |

查看当前 printk 等级配置
cat /proc/sys/kernel/printk
7       4       1       7
| 位置 | 当前值 | 含义 |
| --- | --- | --- |
| 第 1 个 | 7 | 当前控制台日志等级 console_loglevel |
| 第 2 个 | 4 | 默认消息日志等级 default_message_loglevel |
| 第 3 个 | 1 | 最小控制台日志等级 minimum_console_loglevel |
| 第 4 个 | 7 | 默认控制台日志等级 default_console_loglevel |

## Build

```bash
make

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define MY_NAME "chardev"
int major = 0;
char kbuf[128] = {0};
int my_open (struct inode *inode, struct file *file)
{
	printk("open!\n");
	return 0;
}

ssize_t my_read (struct file *file, char __user *ubuf, size_t size, loff_t *offset)
{
	if(size > sizeof(kbuf)) size = sizeof(kbuf);
	if(copy_to_user(ubuf,kbuf,size))
	{
		printk("copy data to user fail!\n");
		return -EIO;
	}
	return size;
}
ssize_t my_write (struct file *file, const char __user *ubuf, size_t size, loff_t *offset)
{
	if(size > sizeof(kbuf)) size = sizeof(kbuf);
	if(copy_from_user(kbuf,ubuf,size))
	{
		printk("copy data form user fail!\n");
		return -EIO;
	}
	return size;
}

int my_close (struct inode *inode, struct file *file)
{
	printk("close!\n");
	return 0;
}

struct file_operations fops = {
	.open = my_open,
	.read = my_read,
	.write = my_write,
	.release = my_close
};

static int __init mycdev_init(void)
{
	major = register_chrdev(0, MY_NAME, &fops);
	if(major < 0)
	{
		printk("reg failed!\n");
		return -1;
	}
	printk("reg successed\n");
    return 0;
}

static void __exit mycdev_exit(void)
{
	printk("hello world %s\n","exit");
	unregister_chrdev(major, MY_NAME);
}

module_init(mycdev_init);

module_exit(mycdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WC WC@gmail.comcomcom");

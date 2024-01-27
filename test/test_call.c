#include <linux/module.h>

void print(void)
{
    printk("test_call is called\n");
}

EXPORT_SYMBOL(print);
MODULE_LICENSE("GPL");

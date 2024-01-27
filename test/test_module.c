#include <linux/module.h>

static int __init test_init(void)
{
    printk("Init module!\n");
    return 0;
}

static void __exit test_exit(void)
{
    printk("Exit module!\n");
}

module_init(test_init);
module_exit(test_exit);
MODULE_LICENSE("GPL");

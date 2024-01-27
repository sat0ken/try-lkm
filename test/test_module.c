#include <linux/module.h>

// 初期化関数
static int __init test_init(void)
{
    printk("Init module!\n");
    return 0;
}

// 終了関数
static void __exit test_exit(void)
{
    printk("Exit module!\n");
}

module_init(test_init); // 初期化関数宣言
module_exit(test_exit); // 終了関数宣言
MODULE_LICENSE("GPL");  // ライセンス宣言

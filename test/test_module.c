#include <linux/module.h>

// test_call.cの関数
void print(void);
static int param = 5;

// 初期化関数
static int __init test_init(void)
{
    printk("Init module! param is %d\n", param);
    print();
    return 0;
}

// 終了関数
static void __exit test_exit(void)
{
    printk("Exit module! param is %d\n", param);
    print();
}

module_init(test_init); // 初期化関数宣言
module_exit(test_exit); // 終了関数宣言
module_param(param, int, 0644);

MODULE_LICENSE("GPL");  // ライセンス宣言
MODULE_AUTHOR("sat0ken<15720506+sat0ken@users.noreply.github.com>");
MODULE_DESCRIPTION("I Do Nothing.");
MODULE_VERSION("1.0");
MODULE_PARM_DESC(param, "parameter of test_module");

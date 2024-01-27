#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>

#define SKEL_MAJOR_NUM 60

// openハンドラ
static int skel_open(struct inode *inode, struct file *file)
{
    printk("SKEL Open\n");
    return 0;
}

// releaseハンドラ
static int skel_release(struct inode *inode, struct file *file)
{
    printk("SKEL Release\n");
    return 0;
}

// writeハンドラ
static ssize_t skel_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    printk("SKEL Write\n");
    return count;
}

// readハンドラ
static ssize_t skel_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    printk("SKEL Read\n");
    return 0;
}

// ファイル操作構造体
static const struct file_operations skel_fops = {
    .owner  = THIS_MODULE,
    .open   = skel_open,
    .release = skel_release,
    .read   = skel_read,
    .write  = skel_write,
};

// class構造体
static struct class skel_class = {
    .owner  = THIS_MODULE,
    .name   = "skel",
};

// probe関数
static int skel_probe(struct platform_device *pdev)
{
    struct device *dev;
    printk("SKEL Probe\n");

    // デバイスファイルを作成する
    dev = device_create(&skel_class, NULL, MKDEV(SKEL_MAJOR_NUM, 0), NULL, "skel");
    if (IS_ERR(dev)) {
        dev_err(&pdev->dev, "failed to create device.\n");
        return PTR_ERR(dev);
    }
    return 0;
}

// remove関数
static int skel_remove(struct platform_device *pdev)
{
    printk("SKEL Remove\n");
    // デバイスファイルを削除する
    device_destroy(&skel_class, MKDEV(SKEL_MAJOR_NUM, 0));
    return 0;
}

// platform_driver構造体
static struct platform_driver skel_driver = {
    .probe  = skel_probe,
    .remove = skel_remove,
    .driver = {
        .name = "skel",
        .owner = THIS_MODULE,
     },
};

// platform_device構造体へのポインタ
static struct platform_device *pdev;

// 初期化関数
static int __init skel_init(void)
{
    int ret = 0;
    printk("SKEL Init\n");

    // クラスの登録
    ret = class_register(&skel_class);
    if (ret != 0) {
        return ret;
    }
    printk("SKEL Init: class_register OK\n");

    // プラットフォームバスにデバイスを登録
    ret = platform_driver_register(&skel_driver);
    if (ret != 0) {
        printk("SKEL Init: platform_driver_register is err %d\n", ret);
        class_unregister(&skel_class);
        return ret;
    }
    printk("SKEL Init: platform_driver_register OK\n");

    // キャラクタドライバの登録
    ret = register_chrdev(SKEL_MAJOR_NUM, "skel", &skel_fops);
    if (ret != 0) {
        printk("SKEL Init: register_chrdev is err %d\n", ret);
        platform_driver_unregister(&skel_driver);
        return ret;
    }
    printk("SKEL Init: register_chrdev OK\n");

    // プラットフォームバスにデバイスを登録
    pdev = platform_device_register_simple("skel", -1, NULL, 0);
    if (!pdev) {
        printk("SKLE Init: platform_device_register_simple is err %d\n", ret);
        unregister_chrdev(SKEL_MAJOR_NUM, "skel");
        return -1;
    }
    printk("SKEL Init: platform_device_register_simple is OK\n");

    return ret;
}

// 終了関数
static void __exit skel_exit(void)
{
    printk("SKEL Exit\n");
    platform_device_unregister(pdev);   // プラットフォームバスからデバイスの登録を解除
    unregister_chrdev(SKEL_MAJOR_NUM, "skel");  // キャラクタ登録を解除
    platform_driver_unregister(&skel_driver);   // プラットフォームバスからドライバの登録を解除
    class_unregister(&skel_class);  // クラス登録の解除
}

module_init(skel_init);
module_exit(skel_exit);
MODULE_LICENSE("GPL");

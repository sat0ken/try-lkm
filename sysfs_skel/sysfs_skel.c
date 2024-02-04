#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>

#define DEV_NAME "skel"
#define SKEL_MAJOR_NUM 60

#define SKEL_MAX 100
#define SKEL_MIN 0

// skelファイルのshow関数
static ssize_t skel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "skel_show called!\n");
}

// skelファイルのstore関数
static ssize_t skel_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    printk("SKEL Write\n");
    return count;
}

// max_skelファイルのshow関数
static ssize_t skel_show_max(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", SKEL_MAX);   
}

// min_skelファイルのshow関数
static ssize_t skel_show_min(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", SKEL_MIN);   
}

static DEVICE_ATTR(skel, 0644, skel_show, skel_store);
static DEVICE_ATTR(skel_max, 0444, skel_show_max, NULL);
static DEVICE_ATTR(skel_min, 0444, skel_show_min, NULL);

// デバイス属性構造体配列
static struct attribute *skel_class_attrs[] = {
    &dev_attr_skel.attr,
    &dev_attr_skel_max.attr,
    &dev_attr_skel_min.attr,
    NULL,
};
ATTRIBUTE_GROUPS(skel_class);

// class構造体
static struct class skel_class = {
    .owner  = THIS_MODULE,
    .name   = DEV_NAME,
    .class_groups = skel_class_groups,
};

// probe関数
static int skel_probe(struct platform_device *pdev)
{
    struct device *dev;
    printk("SKEL Probe\n");

    // 属性ファイルを作成する(デバイスIDを0にすると/devにファイルは作成されない)
    dev = device_create(&skel_class, NULL, 0, NULL, DEV_NAME);
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
    // 属性ファイルを削除する
    device_destroy(&skel_class, 0);
    return 0;
}

// platform_driver構造体
static struct platform_driver skel_driver = {
    .probe  = skel_probe,
    .remove = skel_remove,
    .driver = {
        .name   = DEV_NAME,
        .owner  = THIS_MODULE,
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

    // プラットフォームバスにデバイスを登録
    pdev = platform_device_register_simple(DEV_NAME, -1, NULL, 0);
    if (!pdev) {
        printk("SKLE Init: platform_device_register_simple is err %d\n", ret);
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
    platform_driver_unregister(&skel_driver);   // プラットフォームバスからドライバの登録を解除
    class_unregister(&skel_class);  // クラス登録の解除
}

module_init(skel_init);
module_exit(skel_exit);
MODULE_LICENSE("GPL");

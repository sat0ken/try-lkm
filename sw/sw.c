#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <asm/io.h>

#define DEV_NAME "sw"
#define SW_MAJOR_NUM 60

// raspberry pi 3のペリフェラルIOの物理アドレス
#define BCM2837_PERI_BASE   0x3F000000
#define GPIO_BASE_ADDR      (BCM2837_PERI_BASE + 0x200000)

// GPIO Function Select 1のレジスタ
#define GPFSEL1             0x04

// GPIO Pin Output Set 0のレジスタ
#define GPSET0              0x1C

// GPIO Pin Output Clear 0のレジスタ
#define GPCLR0              0x28

// 使用するSWピン番号
#define SW_PIN             18

// 確保するメモリサイズ
#define MEM_SIZE 0x60

unsigned int *gpio;

// openハンドラ
static int sw_open(struct inode *inode, struct file *file)
{
    printk("SW Open\n");
    return 0;
}

// releaseハンドラ
static int sw_release(struct inode *inode, struct file *file)
{
    printk("SW Release\n");
    return 0;
}

// writeハンドラ
static ssize_t sw_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    printk("SW Write\n");
    return count;
}

// readハンドラ
static ssize_t sw_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    int len, val;
    char *msg;
    printk("SW Read\n");
    
    msg = kmalloc(2, GFP_KERNEL);
    if (msg == NULL) {
        return -ENOMEM;
    }
    request_mem_region(GPIO_BASE_ADDR, MEM_SIZE, DEV_NAME);
    gpio = ioremap(GPIO_BASE_ADDR, MEM_SIZE);

    // SWの状態を読み取る
    val = (gpio[13] & (1 << (SW_PIN % 32))) != 0;
    sprintf(msg, "%d\n", val);
    
    len = strlen(msg);
    if (len - *ppos == 0) {
        return 0;
    }
    if (count > len - *ppos) {
        count = len - *ppos;
    }
    if (copy_to_user(buf, msg, len)) {
        return -EFAULT;
    }
    *ppos += count;


    iounmap((void*)gpio);
    release_mem_region(GPIO_BASE_ADDR, MEM_SIZE);

    return count;
}

// ファイル操作構造体
static const struct file_operations sw_fops = {
    .owner   = THIS_MODULE,
    .open    = sw_open,
    .release = sw_release,
    .read    = sw_read,
    .write   = sw_write,
};

// class構造体
static struct class sw_class = {
    .owner  = THIS_MODULE,
    .name   = DEV_NAME,
};

// probe関数
static int sw_probe(struct platform_device *pdev)
{
    struct device *dev;
    printk("SW Probe\n");

    request_mem_region(GPIO_BASE_ADDR, MEM_SIZE, DEV_NAME);
    gpio = ioremap(GPIO_BASE_ADDR, MEM_SIZE);
    // GPIO18を入力に設定
    gpio[GPFSEL1 / 4] &= ~(0x7 << ((SW_PIN % 10) * 3));
    gpio[GPFSEL1 / 4] |= (0x0 << ((SW_PIN % 10) * 3));

    // デバイスファイルを作成する
    dev = device_create(&sw_class, NULL, MKDEV(SW_MAJOR_NUM, 0), NULL, DEV_NAME);
    if (IS_ERR(dev)) {
        dev_err(&pdev->dev, "faisw to create device.\n");
        return PTR_ERR(dev);
    }
    iounmap((void*)gpio);
    release_mem_region(GPIO_BASE_ADDR, MEM_SIZE);
    return 0;
}

// remove関数
static int sw_remove(struct platform_device *pdev)
{
    printk("SW Remove\n");
    // デバイスファイルを削除する
    device_destroy(&sw_class, MKDEV(SW_MAJOR_NUM, 0));

    return 0;
}

// platform_driver構造体
static struct platform_driver sw_driver = {
    .probe  = sw_probe,
    .remove = sw_remove,
    .driver = {
        .name   = DEV_NAME,
        .owner  = THIS_MODULE,
     },
};

// platform_device構造体へのポインタ
static struct platform_device *pdev;

// 初期化関数
static int __init sw_init(void)
{
    int ret = 0;
    printk("SW Init\n");

    // クラスの登録
    ret = class_register(&sw_class);
    if (ret != 0) {
        return ret;
    }
    printk("SW Init: class_register OK\n");

    // プラットフォームバスにデバイスを登録
    ret = platform_driver_register(&sw_driver);
    if (ret != 0) {
        printk("SW Init: platform_driver_register is err %d\n", ret);
        class_unregister(&sw_class);
        return ret;
    }
    printk("SW Init: platform_driver_register OK\n");

    // キャラクタドライバの登録
    ret = register_chrdev(SW_MAJOR_NUM, DEV_NAME, &sw_fops);
    if (ret != 0) {
        printk("SW Init: register_chrdev is err %d\n", ret);
        platform_driver_unregister(&sw_driver);
        return ret;
    }
    printk("SW Init: register_chrdev OK\n");

    // プラットフォームバスにデバイスを登録
    pdev = platform_device_register_simple(DEV_NAME, -1, NULL, 0);
    if (!pdev) {
        printk("SKLE Init: platform_device_register_simple is err %d\n", ret);
        unregister_chrdev(SW_MAJOR_NUM, "sw");
        return -1;
    }
    printk("SW Init: platform_device_register_simple is OK\n");

    return ret;
}

// 終了関数
static void __exit sw_exit(void)
{
    printk("SW Exit\n");
    platform_device_unregister(pdev);   // プラットフォームバスからデバイスの登録を解除
    unregister_chrdev(SW_MAJOR_NUM, DEV_NAME);  // キャラクタ登録を解除
    platform_driver_unregister(&sw_driver);   // プラットフォームバスからドライバの登録を解除
    class_unregister(&sw_class);  // クラス登録の解除
}

module_init(sw_init);
module_exit(sw_exit);
MODULE_LICENSE("GPL");

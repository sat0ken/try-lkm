#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <asm/io.h>

#define DEV_NAME "led"
#define LED_MAJOR_NUM 60

// raspberry pi 3のペリフェラルIOの物理アドレス
#define BCM2837_PERI_BASE   0x3F000000
#define GPIO_BASE_ADDR      (BCM2837_PERI_BASE + 0x200000)

// GPIO Function Select 1のレジスタ
#define GPFSEL1             0x04

// GPIO Pin Output Set 0のレジスタ
#define GPSET0              0x1C

// GPIO Pin Output Clear 0のレジスタ
#define GPCLR0              0x28

// 使用するLEDピン番号
#define LED_PIN             18

// 確保するメモリサイズ
#define MEM_SIZE 0x60

unsigned int *gpio;

// openハンドラ
static int led_open(struct inode *inode, struct file *file)
{
    printk("LED Open\n");
    return 0;
}

// releaseハンドラ
static int led_release(struct inode *inode, struct file *file)
{
    printk("LED Release\n");
    return 0;
}

// writeハンドラ
static ssize_t led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char *msg;
    printk("LED Write\n");
    msg = kmalloc(2, GFP_KERNEL);
    if (msg == NULL) {
        return -ENOMEM;
    }
    // echoされた文字を取得
    if (copy_from_user(msg + *ppos, buf, count)) {
        return -EFAULT;
    }
    *ppos += count;
    msg[*ppos] = '\0';

    request_mem_region(GPIO_BASE_ADDR, MEM_SIZE, DEV_NAME);
    gpio = ioremap(GPIO_BASE_ADDR, MEM_SIZE);

    if (sysfs_streq("1", msg)) {
        printk("LED Turn on\n");
        // LED点灯
        gpio[GPSET0 / 4] = (1 << (LED_PIN % 32));
    } else {
        printk("LED Turn off\n");
        // LED消灯
        gpio[GPCLR0 / 4] = (1 << (LED_PIN % 32));
    }
 
    iounmap((void*)gpio);
    release_mem_region(GPIO_BASE_ADDR, MEM_SIZE);
    kfree(msg);
    return count;
}

// readハンドラ
static ssize_t led_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    printk("LED Read\n");
    return 0;
}

// ファイル操作構造体
static const struct file_operations led_fops = {
    .owner   = THIS_MODULE,
    .open    = led_open,
    .release = led_release,
    .read    = led_read,
    .write   = led_write,
};

// class構造体
static struct class led_class = {
    .owner  = THIS_MODULE,
    .name   = DEV_NAME,
};

// probe関数
static int led_probe(struct platform_device *pdev)
{
    struct device *dev;
    printk("LED Probe\n");

    request_mem_region(GPIO_BASE_ADDR, MEM_SIZE, DEV_NAME);
    gpio = ioremap(GPIO_BASE_ADDR, MEM_SIZE);
    // GPIO18を出力に設定
    gpio[GPFSEL1 / 4] &= ~(0x7 << ((LED_PIN % 10) * 3));
    gpio[GPFSEL1 / 4] |= (0x1 << ((LED_PIN % 10) * 3));

    // デバイスファイルを作成する
    dev = device_create(&led_class, NULL, MKDEV(LED_MAJOR_NUM, 0), NULL, DEV_NAME);
    if (IS_ERR(dev)) {
        dev_err(&pdev->dev, "failed to create device.\n");
        return PTR_ERR(dev);
    }
    iounmap((void*)gpio);
    release_mem_region(GPIO_BASE_ADDR, MEM_SIZE);
    return 0;
}

// remove関数
static int led_remove(struct platform_device *pdev)
{
    printk("LED Remove\n");
    // デバイスファイルを削除する
    device_destroy(&led_class, MKDEV(LED_MAJOR_NUM, 0));

    request_mem_region(GPIO_BASE_ADDR, MEM_SIZE, DEV_NAME);
    gpio = ioremap(GPIO_BASE_ADDR, MEM_SIZE);
    // Turn off the LED
    gpio[GPCLR0 / 4] = (1 << (LED_PIN % 32));
    iounmap((void*)gpio);
    release_mem_region(GPIO_BASE_ADDR, MEM_SIZE);

    return 0;
}

// platform_driver構造体
static struct platform_driver led_driver = {
    .probe  = led_probe,
    .remove = led_remove,
    .driver = {
        .name   = DEV_NAME,
        .owner  = THIS_MODULE,
     },
};

// platform_device構造体へのポインタ
static struct platform_device *pdev;

// 初期化関数
static int __init led_init(void)
{
    int ret = 0;
    printk("LED Init\n");

    // クラスの登録
    ret = class_register(&led_class);
    if (ret != 0) {
        return ret;
    }
    printk("LED Init: class_register OK\n");

    // プラットフォームバスにデバイスを登録
    ret = platform_driver_register(&led_driver);
    if (ret != 0) {
        printk("LED Init: platform_driver_register is err %d\n", ret);
        class_unregister(&led_class);
        return ret;
    }
    printk("LED Init: platform_driver_register OK\n");

    // キャラクタドライバの登録
    ret = register_chrdev(LED_MAJOR_NUM, DEV_NAME, &led_fops);
    if (ret != 0) {
        printk("LED Init: register_chrdev is err %d\n", ret);
        platform_driver_unregister(&led_driver);
        return ret;
    }
    printk("LED Init: register_chrdev OK\n");

    // プラットフォームバスにデバイスを登録
    pdev = platform_device_register_simple(DEV_NAME, -1, NULL, 0);
    if (!pdev) {
        printk("SKLE Init: platform_device_register_simple is err %d\n", ret);
        unregister_chrdev(LED_MAJOR_NUM, "led");
        return -1;
    }
    printk("LED Init: platform_device_register_simple is OK\n");

    return ret;
}

// 終了関数
static void __exit led_exit(void)
{
    printk("LED Exit\n");
    platform_device_unregister(pdev);   // プラットフォームバスからデバイスの登録を解除
    unregister_chrdev(LED_MAJOR_NUM, DEV_NAME);  // キャラクタ登録を解除
    platform_driver_unregister(&led_driver);   // プラットフォームバスからドライバの登録を解除
    class_unregister(&led_class);  // クラス登録の解除
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");

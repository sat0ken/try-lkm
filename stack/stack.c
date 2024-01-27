#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define DEV_NAME "stack"
#define STACK_MAJOR_NUM 60

#define MAX_MSG_NUM 10      // 最大メッセージ数
#define MAX_MSG_SIZE 20     // 1メッセージの最大サイズ

int msg_num = 0;
char *pmsg[MAX_MSG_NUM];

// openハンドラ
static int stack_open(struct inode *inode, struct file *file)
{
    char *msg;
    printk("STACK Open\n");

    if (file->f_mode & FMODE_WRITE) { // 書き込み時のとき
        if (msg_num == MAX_MSG_NUM) { // スタックが満杯時はエラーを返す
            return -ENOMEM;
        }
        msg = kmalloc(MAX_MSG_SIZE + 1, GFP_KERNEL);    // kmallocでメモリを確保
        if (msg == NULL) {
            return -ENOMEM;
        }
        // 受信したメッセージのポインタをfile構造体のprivate_dataにセット
        // そうすることでread, releaseハンドラでメッセージを取り出す
        file->private_data = msg;
    } else if (file->f_mode & FMODE_READ) {
        if (msg_num == 0) { // スタックが空のときはエラーを返す
            return -ENODATA;
        }
        msg_num--;
        msg = pmsg[msg_num];
        file->private_data = msg;
    } else {
        return -EINVAL;
    }

    return 0;
}

// releaseハンドラ
static int stack_release(struct inode *inode, struct file *file)
{
    // ファイル構造体のprivate_dataに記録していた
    // メッセージのアドレスを取り出す
    char *msg = file->private_data;
    printk("STACK Release\n");

    if (file->f_mode & FMODE_WRITE) {   // 書き込み時
        pmsg[msg_num] = msg;
        msg_num++;
    } else {    // 読み込み時
        kfree(msg);
    }

    return 0;
}

// writeハンドラ
static ssize_t stack_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    // ファイル構造体のprivate_dataに記録していた
    // メッセージのアドレスを取り出す
    char *msg = file->private_data;
    printk("STACK Write\n");

    if (*ppos == MAX_MSG_NUM) { // メッセージが最大サイズの場合はエラーを返す
        return -ENOSPC;
    }
    if (count > MAX_MSG_NUM - *ppos) {
        count = MAX_MSG_NUM - *ppos;
    }
    // ユーザ空間のメッセージデータをカーネル空間にコピー
    if (copy_from_user(msg + *ppos, buf, count)) {
        return -EFAULT;
    }
    *ppos += count; // データ取得した分だけファイルポインタを進める
    msg[*ppos] = '\0';
    printk("message is %s\n", msg);
    
    return count;   // 取得したデータサイズを返す
}

// readハンドラ
static ssize_t stack_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    int len;
    char *msg = file->private_data;
    printk("STACK Read\n");

    len = strlen(msg); // メッセージサイズを取得
    if (len - *ppos == 0) { // メッセージがすべて転送された
        return 0;
    }
    if (count > len - *ppos) {
        count = len - *ppos;
    }
    // カーネル空間からユーザ空間にメッセージデータをコピー
    if (copy_to_user(buf, msg + *ppos, count)) {
        return -EFAULT;
    }
    *ppos += count;

    return count;   // 転送したデータサイズを返す
}

// ファイル操作構造体
static const struct file_operations stack_fops = {
    .owner   = THIS_MODULE,
    .open    = stack_open,
    .release = stack_release,
    .read    = stack_read,
    .write   = stack_write,
};

// class構造体
static struct class stack_class = {
    .owner  = THIS_MODULE,
    .name   = DEV_NAME,
};

// probe関数
static int stack_probe(struct platform_device *pdev)
{
    struct device *dev;
    printk("STACK Probe\n");

    // デバイスファイルを作成する
    dev = device_create(&stack_class, NULL, MKDEV(STACK_MAJOR_NUM, 0), NULL, DEV_NAME);
    if (IS_ERR(dev)) {
        dev_err(&pdev->dev, "failed to create device.\n");
        return PTR_ERR(dev);
    }
    return 0;
}

// remove関数
static int stack_remove(struct platform_device *pdev)
{
    printk("STACK Remove\n");
    // スタックに残っているメッセージをすべて削除してメモリを解放
    while (msg_num) {
        msg_num--;
        kfree(pmsg[msg_num]);
    }
    // デバイスファイルを削除する
    device_destroy(&stack_class, MKDEV(STACK_MAJOR_NUM, 0));
    return 0;
}

// platform_driver構造体
static struct platform_driver stack_driver = {
    .probe  = stack_probe,
    .remove = stack_remove,
    .driver = {
        .name   = DEV_NAME,
        .owner  = THIS_MODULE,
     },
};

// platform_device構造体へのポインタ
static struct platform_device *pdev;

// 初期化関数
static int __init stack_init(void)
{
    int ret = 0;
    printk("STACK Init\n");

    // クラスの登録
    ret = class_register(&stack_class);
    if (ret != 0) {
        return ret;
    }
    printk("STACK Init: class_register OK\n");

    // プラットフォームバスにデバイスを登録
    ret = platform_driver_register(&stack_driver);
    if (ret != 0) {
        printk("STACK Init: platform_driver_register is err %d\n", ret);
        class_unregister(&stack_class);
        return ret;
    }
    printk("STACK Init: platform_driver_register OK\n");

    // キャラクタドライバの登録
    ret = register_chrdev(STACK_MAJOR_NUM, DEV_NAME, &stack_fops);
    if (ret != 0) {
        printk("STACK Init: register_chrdev is err %d\n", ret);
        platform_driver_unregister(&stack_driver);
        return ret;
    }
    printk("STACK Init: register_chrdev OK\n");

    // プラットフォームバスにデバイスを登録
    pdev = platform_device_register_simple(DEV_NAME, -1, NULL, 0);
    if (!pdev) {
        printk("SKLE Init: platform_device_register_simple is err %d\n", ret);
        unregister_chrdev(STACK_MAJOR_NUM, "stack");
        return -1;
    }
    printk("STACK Init: platform_device_register_simple is OK\n");

    return ret;
}

// 終了関数
static void __exit stack_exit(void)
{
    printk("STACK Exit\n");
    platform_device_unregister(pdev);   // プラットフォームバスからデバイスの登録を解除
    unregister_chrdev(STACK_MAJOR_NUM, DEV_NAME);  // キャラクタ登録を解除
    platform_driver_unregister(&stack_driver);   // プラットフォームバスからドライバの登録を解除
    class_unregister(&stack_class);  // クラス登録の解除
}

module_init(stack_init);
module_exit(stack_exit);
MODULE_LICENSE("GPL");

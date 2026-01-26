#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/ktime.h>
#include <linux/kernel.h>
#include <linux/math64.h> /* 必须包含这个头文件才能进行64位除法 */
#include <linux/pinctrl/consumer.h> // 需要包含这个头文件

struct hcsr04_dev {
    int trig_gpio;
    int echo_gpio;
    int irq;
    struct miscdevice misc;
    wait_queue_head_t wq;
    u64 pulse_len;      // 纳秒 (ns)
    int data_ready;     // 数据完成标志
    struct pinctrl *pinctrl; // 添加 pinctrl 结构体指针
};

static irqreturn_t hcsr04_irq(int irq, void *dev_id)
{
    struct hcsr04_dev *d = dev_id;
    static ktime_t start_time;
    ktime_t end_time;

    if (gpio_get_value(d->echo_gpio)) {
        // 上升沿
        start_time = ktime_get();
    } else {
        // 下降沿
        end_time = ktime_get();
        d->pulse_len = ktime_to_ns(ktime_sub(end_time, start_time));
        d->data_ready = 1;
        wake_up_interruptible(&d->wq);
    }
    return IRQ_HANDLED;
}

static ssize_t hcsr04_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    struct hcsr04_dev *d = file->private_data;
    int ret;
    u32 distance_mm;
    u64 math_holder;

    d->data_ready = 0;
    d->pulse_len = 0;

    // 1. 发送 Trigger 信号
    gpio_set_value(d->trig_gpio, 1);
    udelay(15);
    gpio_set_value(d->trig_gpio, 0);

    // 2. 等待回声
    ret = wait_event_interruptible_timeout(d->wq, d->data_ready, msecs_to_jiffies(100));
    if (ret == 0) {
        return -ETIMEDOUT;
    }

    /* * --- 修复部分 ---
     * 原始公式: 距离(mm) = 时间(ns) * 340 / 2 / 1,000,000
     * 为了避免浮点运算和 __aeabi_uldivmod 错误，使用 div_u64
     */
    
    // 第一步：先乘法 (u64 * 340 不会溢出，因为时间通常在ms级)
    math_holder = d->pulse_len * 340;
    
    // 第二步：使用内核专用函数进行 64 位除法
    // 除数是 2000000 (即 2 * 10^9 / 1000 -> 转换 ns 到 mm)
    // distance_mm = math_holder / 2000000; <--- 这是报错的原因
    distance_mm = (u32)div_u64(math_holder, 2000000);

    // 简单的范围过滤 (0 ~ 4.5米)
    if (distance_mm > 4500) distance_mm = 0;

    if (copy_to_user(buf, &distance_mm, sizeof(distance_mm)))
        return -EFAULT;

    return sizeof(distance_mm);
}

static int hcsr04_open(struct inode *inode, struct file *file)
{
    struct hcsr04_dev *d = container_of(file->private_data, struct hcsr04_dev, misc);
    file->private_data = d;
    return 0;
}

static const struct file_operations hcsr04_fops = {
    .owner = THIS_MODULE,
    .read  = hcsr04_read,
    .open  = hcsr04_open,
};

static int hcsr04_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct hcsr04_dev *d;
    int ret;

    d = devm_kzalloc(dev, sizeof(*d), GFP_KERNEL);
    if (!d) return -ENOMEM;

    // >>>>>>>>> 新增 Pin Control 获取代码 <<<<<<<<<
    // 1. 获取并选择 Pin Control 默认状态
    // devm_pinctrl_get_select_default 会查找 pinctrl-names = "default" 对应的状态
    d->pinctrl = devm_pinctrl_get_select_default(dev);
    if (IS_ERR(d->pinctrl)) {
        ret = PTR_ERR(d->pinctrl);
        // 如果 Pin Control 查找失败，通常是 DTS 没写对。
        // 由于 DTS 已经确认正确，这里一般不会失败。
        dev_err(dev, "Failed to get/select default pinctrl state: %d\n", ret);
        // 但这里我们不返回错误，以允许驱动在无 Pin Control 配置下也能尝试运行
        // return ret; // 在您的场景下，为了解决错误，最好返回错误并要求修复DTS
    }
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    d->trig_gpio = of_get_named_gpio(dev->of_node, "trig-gpio", 0);
    d->echo_gpio = of_get_named_gpio(dev->of_node, "echo-gpio", 0);

    if (!gpio_is_valid(d->trig_gpio) || !gpio_is_valid(d->echo_gpio)) {
        dev_err(dev, "GPIO error\n");
        return -EINVAL;
    }

    ret = devm_gpio_request_one(dev, d->trig_gpio, GPIOF_OUT_INIT_LOW, "hcsr04-trig");
    if (ret) return ret;

    ret = devm_gpio_request_one(dev, d->echo_gpio, GPIOF_IN, "hcsr04-echo");
    if (ret) return ret;

    d->irq = gpio_to_irq(d->echo_gpio);
    init_waitqueue_head(&d->wq);

    ret = devm_request_irq(dev, d->irq, hcsr04_irq, 
                           IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, 
                           "hcsr04", d);
                           
    d->misc.minor = MISC_DYNAMIC_MINOR;
    d->misc.name  = "hcsr04";
    d->misc.fops  = &hcsr04_fops;

    ret = misc_register(&d->misc);
    if (ret)
        return ret;

    platform_set_drvdata(pdev, d);   // ⭐⭐⭐ 必须有

    return 0;
}

static int hcsr04_remove(struct platform_device *pdev)
{
    struct hcsr04_dev *d = platform_get_drvdata(pdev);
    misc_deregister(&d->misc);
    return 0;
}

static const struct of_device_id hcsr04_match[] = {
    { .compatible = "alientek,hcsr04" },
    { }
};
MODULE_DEVICE_TABLE(of, hcsr04_match);

static struct platform_driver hcsr04_driver = {
    .probe = hcsr04_probe,
    .remove = hcsr04_remove,
    .driver = {
        .name = "hcsr04",
        .of_match_table = hcsr04_match,
    },
};

module_platform_driver(hcsr04_driver);
MODULE_LICENSE("GPL");
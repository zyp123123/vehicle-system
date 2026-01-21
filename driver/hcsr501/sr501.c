#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#define SR501_NAME "sr501"

static struct gpio_desc *sr501_gpio;
static int sr501_major;
static struct class *sr501_class;

static ssize_t sr501_read(struct file *file, char __user *buf, size_t size, loff_t *off)
{
    int val;
    char data;

    val = gpiod_get_value(sr501_gpio);
    data = val ? '1' : '0';

    if (copy_to_user(buf, &data, 1))
        return -EFAULT;

    return 1;
}

static const struct file_operations sr501_fops = {
    .owner = THIS_MODULE,
    .read  = sr501_read,
};

static int sr501_probe(struct platform_device *pdev)
{
    int ret;

    sr501_gpio = devm_gpiod_get(&pdev->dev, NULL, GPIOD_IN);
    if (IS_ERR(sr501_gpio)) {
        dev_err(&pdev->dev, "Failed to get sr501 GPIO\n");
        return PTR_ERR(sr501_gpio);
    }

    sr501_major = register_chrdev(0, SR501_NAME, &sr501_fops);
    if (sr501_major < 0)
        return sr501_major;

    sr501_class = class_create(THIS_MODULE, SR501_NAME);
    if (IS_ERR(sr501_class)) {
        unregister_chrdev(sr501_major, SR501_NAME);
        return PTR_ERR(sr501_class);
    }

    device_create(sr501_class, NULL, MKDEV(sr501_major, 0), NULL, SR501_NAME);

    dev_info(&pdev->dev, "sr501 sensor driver probed\n");
    return 0;
}

static int sr501_remove(struct platform_device *pdev)
{
    device_destroy(sr501_class, MKDEV(sr501_major, 0));
    class_destroy(sr501_class);
    unregister_chrdev(sr501_major, SR501_NAME);
    return 0;
}

static const struct of_device_id sr501_of_match[] = {
    { .compatible = "hc-sr501" },
    { }
};
MODULE_DEVICE_TABLE(of, sr501_of_match);

static struct platform_driver sr501_driver = {
    .probe  = sr501_probe,
    .remove = sr501_remove,
    .driver = {
        .name = SR501_NAME,
        .of_match_table = sr501_of_match,
    },
};

module_platform_driver(sr501_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("HC-SR501 PIR Motion Sensor Driver");

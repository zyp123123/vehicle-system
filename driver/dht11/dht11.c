#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/mutex.h>

static int dht11_gpio;
static DEFINE_MUTEX(dht11_mutex);

static int dht11_read(int *humidity, int *temperature)
{
    int i, j;
    int data[5] = {0};
    unsigned long timeout;

    mutex_lock(&dht11_mutex);

    gpio_direction_output(dht11_gpio, 0);
    mdelay(20);
    gpio_direction_input(dht11_gpio);

    timeout = jiffies + msecs_to_jiffies(1);
    while (gpio_get_value(dht11_gpio))
        if (time_after(jiffies, timeout)) goto out_err;

    timeout = jiffies + msecs_to_jiffies(1);
    while (!gpio_get_value(dht11_gpio))
        if (time_after(jiffies, timeout)) goto out_err;

    timeout = jiffies + msecs_to_jiffies(1);
    while (gpio_get_value(dht11_gpio))
        if (time_after(jiffies, timeout)) goto out_err;

    for (i = 0; i < 40; i++) {
        while (!gpio_get_value(dht11_gpio));
        udelay(30);
        j = gpio_get_value(dht11_gpio);
        data[i/8] <<= 1;
        if (j) data[i/8] |= 1;
        while (gpio_get_value(dht11_gpio));
    }

    if ((data[0] + data[1] + data[2] + data[3]) & 0xff != data[4])
        goto out_err;

    *humidity = data[0];
    *temperature = data[2];
    mutex_unlock(&dht11_mutex);
    return 0;

out_err:
    mutex_unlock(&dht11_mutex);
    return -EIO;
}

static ssize_t show_value(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
    int hum, temp;
    if (dht11_read(&hum, &temp) == 0)
        return sprintf(buf, "humidity=%d%%, temperature=%dC\n", hum, temp);
    else
        return sprintf(buf, "read failed\n");
}
static DEVICE_ATTR(read, 0444, show_value, NULL);

static int dht11_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;

    dht11_gpio = of_get_named_gpio(np, "dht11-gpio", 0);
    if (!gpio_is_valid(dht11_gpio))
        return -EINVAL;

    if (devm_gpio_request(&pdev->dev, dht11_gpio, "dht11"))
        return -EBUSY;

    device_create_file(&pdev->dev, &dev_attr_read);
    dev_info(&pdev->dev, "DHT11 driver probed, gpio=%d\n", dht11_gpio);
    return 0;
}

static int dht11_remove(struct platform_device *pdev)
{
    device_remove_file(&pdev->dev, &dev_attr_read);
    return 0;
}

static const struct of_device_id dht11_of_match[] = {
    { .compatible = "alientek, dht11"},
    { }
};
MODULE_DEVICE_TABLE(of, dht11_of_match);

static struct platform_driver dht11_driver = {
    .probe  = dht11_probe,
    .remove = dht11_remove,
    .driver = {
        .name = "dht11",
        .of_match_table = dht11_of_match,
    },
};

module_platform_driver(dht11_driver);
MODULE_LICENSE("GPL");

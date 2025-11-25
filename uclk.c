/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#include <uapi/linux/uclk_ioctl.h>

#define UCLK_MAX_CTRLS 64

struct uclk_private {
	const char *name;
	struct clk *clk;
	bool enabled;
};

struct uclk_device {
	unsigned int clocks_count;
	struct miscdevice miscdev;
	struct uclk_private clocks[UCLK_MAX_CTRLS];
};

/* deprecated but kept for backwards compatibility */
static int uclk_ioctl_enable(struct uclk_id *arg)
{
	int code;
	struct uclk_id id;
	struct clk *clk;

	if (copy_from_user(&id, arg, sizeof(struct uclk_id)))
		return -EFAULT;

	clk = clk_get_by_name(id.name);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	code = clk_prepare_enable(clk);

	clk_put(clk);

	return code;
}

/* deprecated but kept for backwards compatibility */
static int uclk_ioctl_disable(struct uclk_id *arg)
{
	struct uclk_id id;
	struct clk *clk;

	if (copy_from_user(&id, arg, sizeof(struct uclk_id)))
		return -EFAULT;

	clk = clk_get_by_name(id.name);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	clk_disable_unprepare(clk);

	clk_put(clk);

	return 0;
}

/* deprecated but kept for backwards compatibility */
static int uclk_ioctl_get_rate(struct uclk_request *arg)
{
	struct uclk_request request;
	struct clk *clk;

	if (copy_from_user(&request, arg, sizeof(struct uclk_request)))
		return -EFAULT;

	clk = clk_get_by_name(request.id.name);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	request.value = clk_get_rate(clk);

	clk_put(clk);

	if (copy_to_user(arg, &request, sizeof(struct uclk_request)))
		return -EFAULT;

	return 0;
}

/* deprecated but kept for backwards compatibility */
static int uclk_ioctl_set_rate(struct uclk_request *arg)
{
	int code;
	struct uclk_request request;
	struct clk *clk;

	if (copy_from_user(&request, arg, sizeof(struct uclk_request)))
		return -EFAULT;

	clk = clk_get_by_name(request.id.name);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	code = clk_set_rate(clk, request.value);

	clk_put(clk);

	return code;
}

static long uclk_ioctl_get_clocks_count(struct uclk_device *uclk_dev,
					__u32 __user *arg)
{
	if (copy_to_user(arg, &uclk_dev->clocks_count, sizeof(*arg)))
		return -EFAULT;

	return 0;
}

static long uclk_ioctl_get_clock_info(struct uclk_device *uclk_dev,
				      struct uclk_info __user *arg)
{
	struct uclk_info info;

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	if (info.id >= uclk_dev->clocks_count)
		return -EINVAL;

	memset(info.name, 0, sizeof(info.name));
	strncpy(info.name, uclk_dev->clocks[info.id].name,
		sizeof(info.name) - 1);

	info.enabled = __clk_is_enabled(uclk_dev->clocks[info.id].clk);

	if (copy_to_user(arg, &info, sizeof(info)))
		return -EFAULT;

	return 0;
}

static long uclk_ioctl_set_clock_state(struct uclk_device *uclk_dev,
				       struct uclk_state __user *arg)
{
	struct uclk_state state;
	int ret = 0;

	if (copy_from_user(&state, arg, sizeof(state)))
		return -EFAULT;

	if (state.id >= uclk_dev->clocks_count)
		return -EINVAL;

	if ((state.state == CLK_STATE_ENABLED) &&
	    (!__clk_is_enabled(uclk_dev->clocks[state.id].clk))) {
		ret = clk_prepare_enable(uclk_dev->clocks[state.id].clk);
		uclk_dev->clocks[state.id].enabled = true;
	} else if ((state.state == CLK_STATE_DISABLED) &&
		   (__clk_is_enabled(uclk_dev->clocks[state.id].clk))) {
		clk_disable_unprepare(uclk_dev->clocks[state.id].clk);
		uclk_dev->clocks[state.id].enabled = false;
	}

	return ret;
}

static long uclk_ioctl_set_clock_rate(struct uclk_device *uclk_dev,
				      struct uclk_rate __user *arg)
{
	struct uclk_rate rate;
	int ret = -EINVAL;

	if (copy_from_user(&rate, arg, sizeof(rate)))
		return -EFAULT;

	if (rate.id >= uclk_dev->clocks_count)
		return -EINVAL;

	ret = clk_set_rate(uclk_dev->clocks[rate.id].clk, rate.rate);

	return ret;
}

static long uclk_ioctl_get_clock_rate(struct uclk_device *uclk_dev,
				      struct uclk_rate __user *arg)
{
	struct uclk_rate rate;

	if (copy_from_user(&rate, arg, sizeof(rate)))
		return -EFAULT;

	if (rate.id >= uclk_dev->clocks_count)
		return -EINVAL;

	rate.rate = clk_get_rate(uclk_dev->clocks[rate.id].clk);

	if (copy_to_user(arg, &rate, sizeof(rate)))
		return -EFAULT;

	return 0;
}

static long uclk_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct miscdevice *uclk_miscdev = file->private_data;
	struct platform_device *pdev =
		container_of(uclk_miscdev->parent, struct platform_device, dev);
	struct uclk_device *uclk_dev = platform_get_drvdata(pdev);

	switch (cmd) {
	/* Deprecated ioctls */
	case UCLK_IOCTL_ENABLE:
		return uclk_ioctl_enable((struct uclk_id *)arg);
	case UCLK_IOCTL_DISABLE:
		return uclk_ioctl_disable((struct uclk_id *)arg);
	case UCLK_IOCTL_GET_RATE:
		return uclk_ioctl_get_rate((struct uclk_request *)arg);
	case UCLK_IOCTL_SET_RATE:
		return uclk_ioctl_set_rate((struct uclk_request *)arg);

	/* Non-deprecated ioctls */
	case UCLK_IOCTL_GET_CLOCKS_COUNT:
		return uclk_ioctl_get_clocks_count(uclk_dev,
						   (__u32 __user *)arg);
	case UCLK_IOCTL_GET_CLOCK_INFO:
		return uclk_ioctl_get_clock_info(
			uclk_dev, (struct uclk_info __user *)arg);
	case UCLK_IOCTL_SET_CLOCK_STATE:
		return uclk_ioctl_set_clock_state(
			uclk_dev, (struct uclk_state __user *)arg);
	case UCLK_IOCTL_SET_CLOCK_RATE:
		return uclk_ioctl_set_clock_rate(
			uclk_dev, (struct uclk_rate __user *)arg);
	case UCLK_IOCTL_GET_CLOCK_RATE:
		return uclk_ioctl_get_clock_rate(
			uclk_dev, (struct uclk_rate __user *)arg);
	default:
		break;
	}

	return -EINVAL;
}

static const struct file_operations uclk_fops = { .owner = THIS_MODULE,
						  .unlocked_ioctl =
							  uclk_ioctl };

static int __init uclk_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct uclk_device *uclk_dev;
	struct device_node *of_node = pdev->dev.of_node;
	struct property *property;
	int num_clock_names, i;

	/* parse device tree for clock-names */
	property = of_find_property(of_node, "clock-names", NULL);
	if (!property) {
		dev_info(
			&pdev->dev,
			"%s has no 'clock-names' property, only legacy clock support.\n",
			of_node->name);
		num_clock_names = 0;
	} else {
		num_clock_names =
			of_property_count_strings(of_node, "clock-names");
		if (num_clock_names < 0)
			return num_clock_names;

		if (num_clock_names > UCLK_MAX_CTRLS)
			num_clock_names = UCLK_MAX_CTRLS;
	}

	uclk_dev = devm_kzalloc(&pdev->dev, sizeof(*uclk_dev), GFP_KERNEL);
	if (!uclk_dev) {
		dev_err(&pdev->dev, "Memory allocation error.\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, uclk_dev);

	if (num_clock_names != 0) {
		dev_info(&pdev->dev, "Registering clocks :\n");

		for (i = 0; i < num_clock_names; i++) {
			/* get clock name from device tree */
			of_property_read_string_index(
				of_node, "clock-names", i,
				&uclk_dev->clocks[i].name);

			if (IS_ERR(uclk_dev->clocks[i].name)) {
				dev_err(&pdev->dev,
					"Could not obtain name clock name at index %d.\n",
					i);
				return PTR_ERR(uclk_dev->clocks[i].name);
			}

			/* try to get clock*/
			uclk_dev->clocks[i].clk = of_clk_get(of_node, i);

			if (IS_ERR(uclk_dev->clocks[i].clk)) {
				dev_err(&pdev->dev,
					"Could not get clock [%s] with index %d.\n",
					uclk_dev->clocks[i].name, i);
				return PTR_ERR(uclk_dev->clocks[i].clk);
			}

			dev_info(&pdev->dev, "clock[%d] = %s\n", i,
				 uclk_dev->clocks[i].name);
		}
	}

	uclk_dev->clocks_count = num_clock_names;

	uclk_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	uclk_dev->miscdev.name = "uclk";
	uclk_dev->miscdev.fops = &uclk_fops;
	uclk_dev->miscdev.parent = &pdev->dev;
	ret = misc_register(&uclk_dev->miscdev);
	if (ret) {
		dev_err(uclk_dev->miscdev.this_device,
			"failed to register misc device %d\n", ret);
		return ret;
	}

	dev_info(&pdev->dev, "registered '%s' %d:%d\n", uclk_dev->miscdev.name,
		 MISC_MAJOR, uclk_dev->miscdev.minor);

	return ret;
}

static int __exit uclk_remove(struct platform_device *pdev)
{
	struct uclk_device *uclk_dev = platform_get_drvdata(pdev);
	int i;

	dev_info(&pdev->dev, "Module exit\n");

	for (i = 0; i < uclk_dev->clocks_count; i++) {
		if (uclk_dev->clocks[i].enabled)
			clk_disable_unprepare(uclk_dev->clocks[i].clk);
		clk_put(uclk_dev->clocks[i].clk);
	}

	misc_deregister(&uclk_dev->miscdev);
	return 0;
}

static const struct of_device_id uclk_dt_match[] = {
	{ .compatible = "coqos,uclk" },
	{ .compatible = "opsy,uclk" },
	{}
};

static struct platform_driver uclk_driver = {
	.driver = {
		.name = "uclk",
		.of_match_table = uclk_dt_match,
	},
	.remove = __exit_p(uclk_remove),
};

module_platform_driver_probe(uclk_driver, uclk_probe);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qualcomm Innovation Center, Inc.");
MODULE_DESCRIPTION("Clocks access driver");

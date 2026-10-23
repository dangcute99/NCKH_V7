// SPDX-License-Identifier: GPL-2.0
/*
 * modem-regs: driver mẫu cho khối thanh ghi AXI-Lite của modem trên PL.
 *
 * Mỗi node DT có compatible = "modem,modem-regs" tạo ra /dev/modem_<địa chỉ>.
 * Offset trong file là offset thanh ghi; đọc/ghi theo word 32-bit căn lề,
 * ví dụ: dd if=/dev/modem_43c00000 bs=4 count=1 skip=2 | hexdump
 */
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

struct modem_regs {
	void __iomem *base;
	resource_size_t size;
	struct miscdevice misc;
};

static struct modem_regs *to_mr(struct file *f)
{
	/* misc_open() đặt private_data trỏ tới miscdevice */
	return container_of(f->private_data, struct modem_regs, misc);
}

/* Số byte được phép truyền từ pos: lỗi nếu lệch 4 byte, 0 nếu đã hết vùng. */
static ssize_t mr_span(struct modem_regs *mr, loff_t pos, size_t len)
{
	if ((pos | len) & 3)
		return -EINVAL;
	if (pos >= mr->size)
		return 0;
	return min_t(resource_size_t, len, mr->size - pos);
}

static ssize_t mr_read(struct file *f, char __user *buf, size_t len, loff_t *pos)
{
	struct modem_regs *mr = to_mr(f);
	ssize_t n = mr_span(mr, *pos, len);
	ssize_t i;

	for (i = 0; i < n; i += 4) {
		u32 v = ioread32(mr->base + *pos + i);

		if (copy_to_user(buf + i, &v, 4))
			return -EFAULT;
	}
	if (n > 0)
		*pos += n;
	return n;
}

static ssize_t mr_write(struct file *f, const char __user *buf, size_t len, loff_t *pos)
{
	struct modem_regs *mr = to_mr(f);
	ssize_t n = mr_span(mr, *pos, len);
	ssize_t i;

	/* ghi quá cuối vùng thanh ghi là lỗi, không lặng lẽ bỏ qua */
	if (n == 0 && len)
		return -ENOSPC;
	for (i = 0; i < n; i += 4) {
		u32 v;

		if (copy_from_user(&v, buf + i, 4))
			return -EFAULT;
		iowrite32(v, mr->base + *pos + i);
	}
	if (n > 0)
		*pos += n;
	return n;
}

static loff_t mr_llseek(struct file *f, loff_t off, int whence)
{
	return fixed_size_llseek(f, off, whence, to_mr(f)->size);
}

static const struct file_operations mr_fops = {
	.owner = THIS_MODULE,
	.read = mr_read,
	.write = mr_write,
	.llseek = mr_llseek,
};

static int mr_probe(struct platform_device *pdev)
{
	struct modem_regs *mr;
	struct resource *res;

	mr = devm_kzalloc(&pdev->dev, sizeof(*mr), GFP_KERNEL);
	if (!mr)
		return -ENOMEM;

	mr->base = devm_platform_get_and_ioremap_resource(pdev, 0, &res);
	if (IS_ERR(mr->base))
		return PTR_ERR(mr->base);
	mr->size = resource_size(res);

	mr->misc.minor = MISC_DYNAMIC_MINOR;
	mr->misc.name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "modem_%llx",
				       (unsigned long long)res->start);
	if (!mr->misc.name)
		return -ENOMEM;
	mr->misc.fops = &mr_fops;
	mr->misc.parent = &pdev->dev;

	platform_set_drvdata(pdev, mr);
	return misc_register(&mr->misc);
}

static int mr_remove(struct platform_device *pdev)
{
	struct modem_regs *mr = platform_get_drvdata(pdev);

	misc_deregister(&mr->misc);
	return 0;
}

static const struct of_device_id mr_of_match[] = {
	{ .compatible = "modem,modem-regs" },
	{ }
};
MODULE_DEVICE_TABLE(of, mr_of_match);

static struct platform_driver mr_driver = {
	.probe = mr_probe,
	.remove = mr_remove,
	.driver = {
		.name = "modem-regs",
		.of_match_table = mr_of_match,
	},
};
module_platform_driver(mr_driver);

MODULE_DESCRIPTION("Driver mẫu cho khối thanh ghi modem trên PL (Zynq)");
MODULE_LICENSE("GPL");

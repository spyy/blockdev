
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/device.h> // class_create, class_destroy
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>



MODULE_LICENSE("Dual BSD/GPL");


// laite luokka
static struct class *dev_class;

// laite numero
static dev_t my_dev;


#define BUFSIZE 256
static char buf[BUFSIZE];

static int gpiobase = 240;
module_param(gpiobase, int, 0444);

#define PIN_SELECT (gpiobase + 0)



#define C_IN(pin) gpio_request_one(pin, GPIOF_IN, #pin)
#define C_OUTH(pin) gpio_request_one(pin, GPIOF_OUT_INIT_HIGH, #pin)
#define C_OUTL(pin) gpio_request_one(pin, GPIOF_OUT_INIT_LOW, #pin)
#define UC(pin) gpio_free(pin)


#define PIN_R (gpiobase + 6)
#define PIN_G (gpiobase + 7)
#define PIN_B (gpiobase + 8)



static void my_gpio_init(void)
{
	C_IN(PIN_SELECT);

	C_OUTH(PIN_R);
	C_OUTH(PIN_G);
	C_OUTH(PIN_B);

}

static void my_gpio_free(void)
{
	UC(PIN_SELECT);
	UC(PIN_R);
	UC(PIN_G);
	UC(PIN_B);
}


static ssize_t my_read(struct file *filp, char __user *ubuff, size_t len, loff_t *offs)
{

	if(len > BUFSIZE) len = BUFSIZE;
	if(!access_ok(VERIFY_WRITE, ubuff, len)) return -EFAULT;

	printk(KERN_ALERT "my_read: offset: %lld\n", *offs);

	if(*offs < 0 || *offs > BUFSIZE){
		printk(KERN_ALERT "my_read: return 0\n");
		return 0;
	}
	else {
		printk(KERN_ALERT "my_read: offset: %lld\n", *offs);
	}


	int remaining = copy_to_user(ubuff, buf, len);

	if(remaining) return -EFAULT;

	return len;
}

static unsigned pin(char c)
{
	switch (c) {
		case 'r':
			return PIN_R;
		case 'g':
			return PIN_G;
		case 'b':
			return PIN_B;
		default:
			printk(KERN_ALERT "pin error");
			return 999;
	};
}


static void process_buffer(void)
{
	int i = 0;
	char color = '0';

	while (buf[i]) {
		switch(buf[i]){
		case 'r':
			color = 'r';
			break;
		case 'g':
			color = 'g';
			break;
		case 'b':
			color = 'b';
			break;
		case '0':
			gpio_set_value(pin(color), 0);
			break;
		case '1':
			gpio_set_value(pin(color), 1);
			break;
		default:
			color = '0';
			break;
		};
		i++;
	};
}

static ssize_t my_write(struct file *filp, const char __user *ubuff, size_t len, loff_t *offs)
{
	printk(KERN_ALERT "my_write\n");

	if(len > BUFSIZE) len = BUFSIZE;
	if(!access_ok(VERIFY_READ, ubuff, len)) return -EFAULT;

	int remaining = copy_from_user(buf, ubuff, len);
	if(remaining) return -EFAULT;

	process_buffer();
	memset(buf, 0, BUFSIZE);

	return len;
}

static int my_open(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "my_open\n");
	return 0;
}

static int my_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "my_release\n");
	return 0;
}

long my_unlocked_ioctl(struct file *d, unsigned int request, unsigned long data){
	printk(KERN_ALERT "my_unlocked_ioctl: request %x, data %lx", request, data);
	return 0;
}


static struct file_operations my_fileops = {
.read = my_read,
.write = my_write,
.open = my_open,
.release = my_release,
.unlocked_ioctl = my_unlocked_ioctl
};


// laite
static struct cdev my_cdev;
static struct device *my_device;

static int virtualdev_init(void)
{
	int err;

	// 1 class
	dev_class  = class_create(THIS_MODULE, "block-class");
	if(IS_ERR(dev_class)) return -1;

	// 2 char device region
	err = alloc_chrdev_region(&my_dev, 0, 1, "block-region");
	if (err) goto alloc_region_err;
	// 3 initialize device
	cdev_init(&my_cdev, &my_fileops);
	my_cdev.owner = THIS_MODULE;
	// 4 add device
	err = cdev_add(&my_cdev, my_dev, 1);
	if(err) goto add_err;
	// 5 create device
	my_device = device_create(dev_class, NULL, my_dev, NULL, "block_dev");
	if(IS_ERR(my_device)) goto device_err;

	my_gpio_init();

	return 0;

	device_err: device_destroy(dev_class, my_dev); err = -1;
	add_err: cdev_del(&my_cdev);
	alloc_region_err: unregister_chrdev_region(my_dev, 1);

    return err;
}

static void virtualdev_exit(void)
{
	device_destroy(dev_class, my_dev);
	cdev_del(&my_cdev);
	unregister_chrdev_region(my_dev, 1);
	class_destroy(dev_class);

	my_gpio_free();
}

module_init(virtualdev_init);
module_exit(virtualdev_exit);




#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>


MODULE_LICENSE("GPL");

static unsigned int gpioLED1 = 20;
static unsigned int gpioLED2 = 16;
static unsigned int gpioButton1 = 21;
static unsigned int gpioButton2 = 13;
static unsigned int gpioButton3 = 19;
static unsigned int gpioButton4 = 26;
static unsigned int irqNumber1;
static unsigned int irqNumber2;
static unsigned int irqNumber3;
static unsigned int irqNumber4;
static unsigned int numberPresses = 0;
static bool ledOn_1 = false;
static bool ledOn_2 = false;

static int device_file_b1_major_number = 0;
static int device_file_b2_major_number = 0;
static int device_file_b3_major_number = 0;
static int device_file_b4_major_number = 0;
static const char device_name_button1[] = "ASO BUTTON 1: ";
static const char device_name_button2[] = "ASO BUTTON 2: ";
static const char device_name_button3[] = "ASO BUTTON 3: ";
static const char device_name_button4[] = "ASO BUTTON 4: ";


static irq_handler_t ebbgpio_irq_handler1(unsigned int irq, void* dev_id, struct pt_regs* regs);
static irq_handler_t ebbgpio_irq_handler2(unsigned int irq, void* dev_id, struct pt_regs* regs);
static irq_handler_t ebbgpio_irq_handler3(unsigned int irq, void* dev_id, struct pt_regs* regs);
static irq_handler_t ebbgpio_irq_handler4(unsigned int irq, void* dev_id, struct pt_regs* regs);


//Funcions de read de cada character device:
static ssize_t device_file_read_b1(struct file* file_ptr, char __user* user_buffer, size_t count, loff_t* position)
{
	printk(KERN_NOTICE "ASO: button 1 read!\n");
	return 0;
}

static ssize_t device_file_read_b2(struct file* file_ptr, char __user* user_buffer, size_t count, loff_t* position)
{
	printk(KERN_NOTICE "ASO: button 2 read!\n");
	return 0;
}

static ssize_t device_file_read_b3(struct file* file_ptr, char __user* user_buffer, size_t count, loff_t* position)
{
	printk(KERN_NOTICE "ASO: button 3 read!\n");
	return 0;
}

static ssize_t device_file_read_b4(struct file* file_ptr, char __user* user_buffer, size_t count, loff_t* position)
{
	printk(KERN_NOTICE "ASO: button 4 read!\n");
	return 0;
}

//Structs per les operacions dels fitxers:
static struct file_operations simple_driver_fops1 =
{
	.owner = THIS_MODULE,
	.read = device_file_read_b1,
};

static struct file_operations simple_driver_fops2 =
{
	.owner = THIS_MODULE,
	.read = device_file_read_b2,
};

static struct file_operations simple_driver_fops3 =
{
	.owner = THIS_MODULE,
	.read = device_file_read_b3,
};

static struct file_operations simple_driver_fops4 =
{
	.owner = THIS_MODULE,
	.read = device_file_read_b4,
};

static int register_device_in(unsigned int gpioButton, const char* device_name, struct file_operations* simple_driver_fops,
	int* device_file)
{
	int result = 0;

	printk(KERN_NOTICE "ASO: register button (gpio %d)\n", gpioButton);
	if (!gpio_is_valid(gpioButton))
	{
		printk(KERN_NOTICE "ASO GPIO: invalid in gpio num: %d", gpioButton);
		return -ENODEV;
	}

	//Button ON LED, input
	gpio_request(gpioButton, "sysfs");
	gpio_direction_input(gpioButton);
	gpio_set_debounce(gpioButton, 200);
	gpio_export(gpioButton, false);

	printk(KERN_NOTICE "ASO GPIO: button current state: %d", gpio_get_value(gpioButton));

	//Registering character device
	result = register_chrdev(0, device_name, simple_driver_fops);
	if (result < 0)
	{
		printk(KERN_WARNING "ASO: can't register character device with error code = %i\n", result);
		return result;
	}
	*device_file = result;
	printk(KERN_NOTICE "ASO: Device file: %i\n", *device_file);

	return 0;


}

static int register_device_out(unsigned int gpioLED)
{
	printk(KERN_NOTICE "ASO: registering LED (gpio %d)\n", gpioLED);

	if (!gpio_is_valid(gpioLED))
	{
		printk(KERN_NOTICE "ASO GPIO: invalid out gpio num: %d", gpioLED);
		return -ENODEV;
	}

	//GPIO commands:
	//LED1 GPIO, Output
	gpio_request(gpioLED, "sysfs");
	gpio_direction_output(gpioLED, false);
	gpio_export(gpioLED, false);

	return 0;
}

static void register_interrupts(void)
{
	int result;
	//Setting up the interrupt 1
	irqNumber1 = gpio_to_irq(gpioButton1);
	printk(KERN_NOTICE "ASO GPIO: button mapped to %d irq", irqNumber1);

	result = request_irq(irqNumber1,
		(irq_handler_t)ebbgpio_irq_handler1,
		IRQF_TRIGGER_FALLING,
		"ebb_gpio_handler",
		NULL);
	printk(KERN_NOTICE "ASO GPIO: interrupt request result is: %d\n", result);

	//Setting up the interrupt 2
	irqNumber2 = gpio_to_irq(gpioButton2);
	printk(KERN_NOTICE "ASO GPIO: button mapped to %d irq", irqNumber2);

	result = request_irq(irqNumber2,
		(irq_handler_t)ebbgpio_irq_handler2,
		IRQF_TRIGGER_FALLING,
		"ebb_gpio_handler",
		NULL);
	printk(KERN_NOTICE "ASO GPIO: interrupt request result is: %d\n", result);

	//Setting up the interrupt 3
	irqNumber3 = gpio_to_irq(gpioButton3);
	printk(KERN_NOTICE "ASO GPIO: button mapped to %d irq", irqNumber3);

	result = request_irq(irqNumber3,
		(irq_handler_t)ebbgpio_irq_handler3,
		IRQF_TRIGGER_FALLING,
		"ebb_gpio_handler",
		NULL);
	printk(KERN_NOTICE "ASO GPIO: interrupt request result is: %d\n", result);

	//Setting up the interrupt 2
	irqNumber4 = gpio_to_irq(gpioButton4);
	printk(KERN_NOTICE "ASO GPIO: button mapped to %d irq", irqNumber4);

	result = request_irq(irqNumber4,
		(irq_handler_t)ebbgpio_irq_handler4,
		IRQF_TRIGGER_FALLING,
		"ebb_gpio_handler",
		NULL);
	printk(KERN_NOTICE "ASO GPIO: interrupt request result is: %d\n", result);
}


static void unregister_device(void)
{
	printk(KERN_NOTICE "ASO: unregister device() is called\n");
	printk(KERN_NOTICE "ASO GPIO: the button was pressed %d times\n", numberPresses);
	gpio_set_value(gpioLED1, 0);
	gpio_set_value(gpioLED2, 0);
	gpio_unexport(gpioLED1);
	gpio_unexport(gpioLED2);
	free_irq(irqNumber1, NULL);
	free_irq(irqNumber2, NULL);
	free_irq(irqNumber3, NULL);
	free_irq(irqNumber4, NULL);
	gpio_unexport(gpioButton1);
	gpio_unexport(gpioButton2);
	gpio_unexport(gpioButton3);
	gpio_unexport(gpioButton4);
	gpio_free(gpioLED1);
	gpio_free(gpioLED2);
	gpio_free(gpioButton1);
	gpio_free(gpioButton2);
	gpio_free(gpioButton3);
	gpio_free(gpioButton4);
	printk(KERN_NOTICE "ASO GPIO: gpios have been freed.\n");

	if (device_file_b1_major_number != 0)
	{
		unregister_chrdev(device_file_b1_major_number, device_name_button1);
	}

	if (device_file_b2_major_number != 0)
	{
		unregister_chrdev(device_file_b2_major_number, device_name_button2);
	}

	if (device_file_b3_major_number != 0)
	{
		unregister_chrdev(device_file_b3_major_number, device_name_button3);
	}

	if (device_file_b4_major_number != 0)
	{
		unregister_chrdev(device_file_b4_major_number, device_name_button4);
	}

	printk(KERN_NOTICE "ASO CHAR: character devices have been freed.\n");

}

static irq_handler_t ebbgpio_irq_handler1(unsigned int irq, void* dev_id, struct pt_regs* regs)
{
	ledOn_1 = true;
	gpio_set_value(gpioLED1, ledOn_1);
	printk(KERN_NOTICE "ASO GPIO: Button pressed! Value: %d", gpio_get_value(gpioButton1));
	numberPresses++;
	return (irq_handler_t)IRQ_HANDLED;

}

static irq_handler_t ebbgpio_irq_handler2(unsigned int irq, void* dev_id, struct pt_regs* regs)
{
	ledOn_1 = false;
	gpio_set_value(gpioLED1, ledOn_1);
	printk(KERN_NOTICE "ASO GPIO: Button pressed! Value: %d", gpio_get_value(gpioButton2));
	numberPresses++;
	return (irq_handler_t)IRQ_HANDLED;

}

static irq_handler_t ebbgpio_irq_handler3(unsigned int irq, void* dev_id, struct pt_regs* regs)
{
	ledOn_2 = true;
	gpio_set_value(gpioLED2, ledOn_2);
	printk(KERN_NOTICE "ASO GPIO: Button pressed! Value: %d", gpio_get_value(gpioButton3));
	numberPresses++;
	return (irq_handler_t)IRQ_HANDLED;

}

static irq_handler_t ebbgpio_irq_handler4(unsigned int irq, void* dev_id, struct pt_regs* regs)
{
	ledOn_2 = false;
	gpio_set_value(gpioLED2, ledOn_2);
	printk(KERN_NOTICE "ASO GPIO: Button pressed! Value: %d", gpio_get_value(gpioButton4));
	numberPresses++;
	return (irq_handler_t)IRQ_HANDLED;

}

static int __init my_init(void) {
	printk(KERN_NOTICE "Init LKM\n");
	register_device_in(gpioButton1, device_name_button1, &simple_driver_fops1, &device_file_b1_major_number);
	register_device_in(gpioButton2, device_name_button2, &simple_driver_fops2, &device_file_b2_major_number);
	register_device_in(gpioButton3, device_name_button3, &simple_driver_fops3, &device_file_b3_major_number);
	register_device_in(gpioButton4, device_name_button4, &simple_driver_fops4, &device_file_b4_major_number);
	register_device_out(gpioLED1);
	register_device_out(gpioLED2);
	register_interrupts();
	return 0;
}

static void __exit my_exit(void) {
	printk(KERN_NOTICE "Exit LKM\n");
	unregister_device();
	return;
}

module_init(my_init);
module_exit(my_exit);

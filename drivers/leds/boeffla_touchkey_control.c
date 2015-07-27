/*
 * Author: andip71, 21.09.2014
 *
 * Version 1.0.0
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * Change log:
 *
 * 1.0.0 (27.07.2015)
 *   - Initial version for OnePlus
 *
 * 		Sys fs:
 *
 * 			/sys/class/misc/btk_control/btkc_mode
 *
 * 				0: normal mode, touch key lights controlled by rom only
 *
 * 				1: touch key buttons only - touch key lights only active
 * 					when touch keys pressed
 *
 * 				2: Off - touch key lights are always off
 *
 *
 * 			/sys/class/misc/btk_control/btkc_version
 *
 * 				display version of Boeffla touch key control driver
 *
 */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/boeffla_touchkey_control.h>


/*****************************************/
// Variables
/*****************************************/

int btkc_mode = MODE_NORMAL;
struct led_classdev *touch_led = NULL;


/*****************************************/
// exported functions
/*****************************************/

void btkc_touch(int x, int y)
{
	printk("Boeffla touch key control: touch detected %d %d\n", x, y);

	// only control LED (activation) when touchkey only mode
	if (btkc_mode != MODE_TOUCHKEY_ONLY)
		return;

	// touch key with finger 0 received, check if a touch key was pressed
	// if yes, enable touch key backlight
	if ((y > TOUCHKEY_Y) && (touch_led != NULL))
		led_brightness_set(touch_led, 255);
}


int btkc_block_touchkey_backlight(unsigned long state)
{
	// normal mode = never block the LED
	if (btkc_mode == MODE_NORMAL)
		return 0;

	// mode set to touchkey only or off = always block enabling the LED
	if (state != LED_OFF)
		return 1;

	return 0;
}


void btkc_store_handle(struct led_classdev *led_cdev)
{
	printk("Boeffla touch key control: Led handle received\n");
	touch_led = led_cdev;
	return;
}


/*****************************************/
// sysfs interface functions
/*****************************************/

static ssize_t btkc_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Mode: %d\n", btkc_mode);
}


static ssize_t btkc_mode_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	unsigned int ret = -EINVAL;
	unsigned int val;

	// check data and store if valid
	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;

	if ((val >= 0) || (val <= 2))
	{
		btkc_mode = val;

		// disable touchkey backlight after every mode change
		if (touch_led != NULL)
			led_brightness_set(touch_led, 0);
	}

	return count;
}


static ssize_t btkc_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", BTK_CONTROL_VERSION);
}


/***************************************************/
// Initialize boeffla touch key control sysfs folder
/***************************************************/

// define objects
static DEVICE_ATTR(btkc_mode, S_IRUGO | S_IWUGO, btkc_mode_show, btkc_mode_store);
static DEVICE_ATTR(btkc_version, S_IRUGO | S_IWUGO, btkc_version_show, NULL);

// define attributes
static struct attribute *btkc_attributes[] = {
	&dev_attr_btkc_mode.attr,
	&dev_attr_btkc_version.attr,
	NULL
};

// define attribute group
static struct attribute_group btkc_control_group = {
	.attrs = btkc_attributes,
};

// define control device
static struct miscdevice btkc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "btk_control",
};


/*****************************************/
// Driver init and exit functions
/*****************************************/

static int btk_control_init(void)
{
	// register boeffla touch key control device
	misc_register(&btkc_device);
	if (sysfs_create_group(&btkc_device.this_device->kobj,
				&btkc_control_group) < 0) {
		printk("Boeffla touch key control: failed to create sys fs object.\n");
		return 0;
	}

	// Print debug info
	printk("Boeffla touch key control: driver version %s started\n", BTK_CONTROL_VERSION);
	return 0;
}


static void btk_control_exit(void)
{
	// Print debug info
	printk("Boeffla touch key control: driver stopped\n");
}


/* define driver entry points */
module_init(btk_control_init);
module_exit(btk_control_exit);

MODULE_AUTHOR("andip71");
MODULE_DESCRIPTION("boeffla touch key control");
MODULE_LICENSE("GPL v2");

/*
 * 
 * Boeffla touchkey control OnePlus One
 *
 * Author: andip71 (aka Lord Boeffla)
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
 * 1.1.0 (29.07.2015)
 * 	- Add full kernel handling with configurable timeout
 *
 * 		Sys fs:
 *
 * 			/sys/class/misc/btk_control/btkc_timeout
 *
 * 				timeout in seconds from 1 - 30
 * 				(0 = rom is taking care of the timeout)
 *
 *
 * 1.0.0 (27.07.2015)
 *	- Initial version
 *
 *		Sys fs:
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
#include <linux/workqueue.h>
#include <linux/notifier.h>
#include <linux/kobject.h>
#include <linux/lcd_notify.h>
#include <linux/boeffla_touchkey_control.h>


/*****************************************/
// Declarations
/*****************************************/

int btkc_mode    = MODE_NORMAL;		// normal mode is default
int btkc_timeout = TIMEOUT_DEFAULT;	// default is rom controlled timeout

struct led_classdev *touch_led = NULL;

static struct notifier_block lcd_notif;

static void led_work_func(struct work_struct *unused);
static DECLARE_DELAYED_WORK(led_work, led_work_func);


/*****************************************/
// internal functions
/*****************************************/

static void touchkey_led_enable(void)
{
	if (touch_led != NULL)
		led_brightness_set(touch_led, LED_ON);
}

static void touchkey_led_disable(void)
{
	if (touch_led != NULL)
		led_brightness_set(touch_led, LED_OFF);
}

static void led_work_func(struct work_struct *unused)
{
	pr_debug("Boeffla touch key control: timeout over, disable touchkey led");

	// switch off LED and cancel any scheduled work
	touchkey_led_disable();
	cancel_delayed_work(&led_work);
}

static int lcd_notifier_callback(struct notifier_block *this,
								unsigned long event, void *data)
{
	switch (event)
	{
		case LCD_EVENT_OFF_START:
			pr_debug("Boeffla touch key control: screen off detected, disable touchkey led");
			// switch off LED and cancel any scheduled work
			touchkey_led_disable();
			cancel_delayed_work(&led_work);
			break;

		case LCD_EVENT_ON_END:
			break;

		default:
			break;
	}

	return 0;
}


/*****************************************/
// exported functions
/*****************************************/

void btkc_touch(int x, int y)
{
	pr_debug("Boeffla touch key control: touch detected %d %d\n", x, y);

	// no special handling when not in touchkey_only mode
	if (btkc_mode != MODE_TOUCHKEY_ONLY)
		return;

	// touch event with finger 0 received, check if a touch key was pressed;
	// if yes, enable touch key led
	if (y > TOUCHKEY_Y)
	{
		touchkey_led_enable();

		// only if kernel should take over full control of the timeout,
		// schedule the required delayed work to switch LED off again
		if (btkc_timeout > 0)
		{
			cancel_delayed_work(&led_work);
			schedule_delayed_work(&led_work, msecs_to_jiffies(btkc_timeout * 1000));
		}
	}
}

int btkc_block_touchkey_backlight(unsigned long state)
{
	pr_debug("Boeffla touch key control: Led sysfs hook - status %lu\n", state);

	// normal mode = never block the LED
	if (btkc_mode == MODE_NORMAL)
		return false;

	// mode set to touchkey_only or off = always block enabling the LED
	if (state != LED_OFF)
		return true;

	// if kernel has timeout control for touchkey_only mode = block LED completely
	if ((btkc_mode == MODE_TOUCHKEY_ONLY) && (btkc_timeout > 0))
		return true;

	// do not block disabling the LED otherwise
	return false;
}

void btkc_store_handle(struct led_classdev *led_cdev)
{
	// handle received by hook, store it
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

	if ((val >= MODE_NORMAL) || (val <= MODE_OFF))
	{
		btkc_mode = val;

		// reset LED after every mode change
		cancel_delayed_work(&led_work);
		touchkey_led_disable();
	}

	return count;
}


static ssize_t btkc_timeout_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Timeout [s]: %d\n", btkc_timeout);
}

static ssize_t btkc_timeout_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	unsigned int ret = -EINVAL;
	unsigned int val;

	// check data and store if valid
	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;

	if ((val >= TIMEOUT_MIN) || (val <= TIMEOUT_MAX))
	{
		btkc_timeout = val;

		// reset LED after every timeout change
		cancel_delayed_work(&led_work);
		touchkey_led_disable();
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
static DEVICE_ATTR(btkc_timeout, S_IRUGO | S_IWUGO, btkc_timeout_show, btkc_timeout_store);
static DEVICE_ATTR(btkc_version, S_IRUGO | S_IWUGO, btkc_version_show, NULL);

// define attributes
static struct attribute *btkc_attributes[] = {
	&dev_attr_btkc_mode.attr,
	&dev_attr_btkc_timeout.attr,
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

	// register callback for screen on/off notifier
	lcd_notif.notifier_call = lcd_notifier_callback;
	if (lcd_register_client(&lcd_notif) != 0)
		pr_err("%s: Failed to register lcd callback\n", __func__);

	// Print debug info
	printk("Boeffla touch key control: driver version %s started\n", BTK_CONTROL_VERSION);
	return 0;
}


static void btk_control_exit(void)
{
	// remove boeffla touch key control device
	sysfs_remove_group(&btkc_device.this_device->kobj,
                           &btkc_control_group);

	// cancel and flush any remaining scheduled work
	cancel_delayed_work(&led_work);
	flush_scheduled_work();

	// unregister screen notifier
	lcd_unregister_client(&lcd_notif);

	// Print debug info
	printk("Boeffla touch key control: driver stopped\n");
}


/* define driver entry points */
module_init(btk_control_init);
module_exit(btk_control_exit);

MODULE_AUTHOR("andip71");
MODULE_DESCRIPTION("boeffla touch key control");
MODULE_LICENSE("GPL v2");

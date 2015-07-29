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

#include <linux/leds.h>


/*****************************************/
// Definitions
/*****************************************/

#define	MODE_NORMAL			0
#define MODE_TOUCHKEY_ONLY	1
#define MODE_OFF			2

#define TOUCHKEY_Y			1900

#define TIMEOUT_DEFAULT		0
#define TIMEOUT_MIN			0
#define TIMEOUT_MAX			30

#define LED_OFF				0
#define LED_ON				255

#define BTK_CONTROL_VERSION 	"1.1.0"


/*****************************************/
// Function declarations
/*****************************************/

void btkc_store_handle(struct led_classdev *led);
int btkc_block_touchkey_backlight(unsigned long state);
void btkc_touch(int x, int y);


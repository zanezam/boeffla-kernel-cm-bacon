/*
 * Author: andip71, 27.07.2015
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
 
#include <linux/leds.h>


/*****************************************/
// Definitions
/*****************************************/

#define	MODE_NORMAL			0
#define MODE_TOUCHKEY_ONLY	1
#define MODE_OFF			2

#define TOUCHKEY_Y			1900

#define BTK_CONTROL_VERSION 	"1.0.0"


/*****************************************/
// Function declarations
/*****************************************/

void btkc_store_handle(struct led_classdev *led);
int btkc_block_touchkey_backlight(unsigned long state);
void btkc_touch(int x, int y);


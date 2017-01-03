/*
 * Linux 2.6 and later 'muon_timer' device driver
 *
 * Copyright (c) 2017, Kevin Lynch <klynch@york.cuny.edu>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Lynch");
MODULE_DESCRIPTION("An interrupt-based timing measurement driver for the BBB");
MODULE_VERSION("0.1");



static int __init muon_timer_init(void){
  return 0;
}

static void __exit muon_timer_exit(void){

}


module_init(muon_timer_init);
module_exit(muon_timer_exit);

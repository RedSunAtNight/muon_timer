#include <linux/module.h>
#include <linux/init.h>
//#include <linux/kernel.h>
//#include <linux/gpio.h>
//#include <linux/interrupt.h>
//#include <linux/moduleparam.h>

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

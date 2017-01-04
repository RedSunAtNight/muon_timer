#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Lynch");
MODULE_DESCRIPTION("An interrupt-based timing measurement driver for the BBB");
MODULE_VERSION("1.0");

// Macros swiped from parrot driver
#define dbg(format, arg...) do { if (debug) pr_info(CLASS_NAME ": %s: " format , __FUNCTION__ , ## arg); } while (0)
#define err(format, arg...) pr_err(CLASS_NAME ": " format, ## arg)
#define info(format, arg...) pr_info(CLASS_NAME ": " format, ## arg)
#define warn(format, arg...) pr_warn(CLASS_NAME ": " format, ## arg)

#define DEVICE_NAME "muon_timer_device"
#define CLASS_NAME "muon_timer"


// module parameters appear in /sys/module/muon_timer/parameters
// add 'debug=1' on insmod to change .. 
///
static bool debug = false;
module_param(debug, bool, S_IRUGO);
MODULE_PARM_DESC(debug, "enable debug info (default: false)");
///
static int gpio_input = 115;  // BBB GPIO P9_27
module_param(gpio_input, int, S_IRUGO);
MODULE_PARM_DESC(gpio_input, "BBB GPIO line for discriminated input (default: 115)");
///
static int gpio_reset  = 48; // BBB GPIO ???
module_param(gpio_reset, int, S_IRUGO);
MODULE_PARM_DESC(gpio_reset, "BBB GPIO line for latch reset output (default: 48)");
///
static int gpio_pulse = 49; // BBB GPIO P9_23
module_param(gpio_pulse, int, S_IRUGO);
MODULE_PARM_DESC(gpio_pulse, "BBB GPIO line for user output (default: 49)");

static int muon_major;
static struct class *muon_class = 0;
static struct device *muon_device = 0;

static struct file_operations fops = {};

static int __init muon_timer_init(void){
  int retval = 0;

  dbg("");
  // Register char device and get a muon_major number
  muon_major = register_chrdev(0, DEVICE_NAME, &fops);
  if( muon_major<0 ){
    err("failed to register char device: error %d\n", muon_major);
    retval = muon_major;
    goto failed_chrdev;
  }

  // Register the muon_timer virtual class ... no physical bus to
  // connect to
  muon_class = class_create(THIS_MODULE, CLASS_NAME);
  if(IS_ERR(muon_class)){
    err("failed to register device class '%s'\n", CLASS_NAME);
    retval = PTR_ERR(muon_class);
    goto failed_class;
  }

  // create device and nodes in /dev
  muon_device = device_create(muon_class, NULL, MKDEV(muon_major, 0),
			      NULL, CLASS_NAME);
  if(IS_ERR(muon_device)){
    err("failed to create device '%s'\n", CLASS_NAME);
    retval = PTR_ERR(muon_device);
    goto failed_create;
  }
  
  return 0;

 failed_create:
  class_destroy(muon_class);
 failed_class:
  unregister_chrdev(muon_major, DEVICE_NAME);
 failed_chrdev:
  return retval;
}

static void __exit muon_timer_exit(void){
  dbg("");

  device_destroy(muon_class, MKDEV(muon_major,0));
  class_destroy(muon_class);
  unregister_chrdev(muon_major, DEVICE_NAME);
}


module_init(muon_timer_init);
module_exit(muon_timer_exit);

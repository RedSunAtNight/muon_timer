#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/time.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Lynch");
MODULE_DESCRIPTION("An interrupt-based timing measurement driver for the BBB");
MODULE_VERSION("1.0");

// Macros swiped from parrot driver ... 
//// FIXME: not sure why they work inconsistently ... flushing?
#define dbg(format, arg...) do { if (debug) pr_info(CLASS_NAME ": %s: " format "\n", __FUNCTION__ , ## arg); } while (0)
#define err(format, arg...) pr_err(CLASS_NAME ": " format "\n", ## arg)
#define info(format, arg...) pr_info(CLASS_NAME ": " format "\n", ## arg)
#define warn(format, arg...) pr_warn(CLASS_NAME ": " format "\n", ## arg)

#define DEVICE_NAME "muon_timer_device"
#define CLASS_NAME "muon_timer"
#define MUON_TIMER_FIFO_SIZE 1024

// module parameters appear in /sys/module/muon_timer/parameters
// add 'debug=1' on insmod to change .. 
///g
static bool debug = false;
module_param(debug, bool, S_IRUGO);
MODULE_PARM_DESC(debug, "enable debug info (default: false)");
///
static unsigned int gpio_reset  = 48; // BBB GPIO P9_15
module_param(gpio_reset, uint, S_IRUGO);
MODULE_PARM_DESC(gpio_reset, "BBB GPIO line for latch reset output (default: 48)");
///
static unsigned int gpio_pulse = 49; // BBB GPIO P9_23
module_param(gpio_pulse, uint, S_IRUGO);
MODULE_PARM_DESC(gpio_pulse, "BBB GPIO line for user output (default: 49)");
///
static unsigned int gpio_input = 115;  // BBB GPIO P9_27
module_param(gpio_input, uint, S_IRUGO);
MODULE_PARM_DESC(gpio_input, "BBB GPIO line for discriminated input (default: 115)");

static int muon_major;
static struct class *muon_class = 0;
static struct device *muon_device = 0;
static int irq;

// We'll allow only one process into the timer at once
static DEFINE_MUTEX(muon_timer_mutex);

// define fifo ... 
static DEFINE_KFIFO(muon_timer_fifo, struct timeval, MUON_TIMER_FIFO_SIZE);
// going to need a recovery timer, too
// make pulse
void do_make_pulse(unsigned pin){
  unsigned long flags;

  dbg("enter");
  // we're bit banging ... need to disable preemption and local irqs
  // so this doesn't take an unbounded time...
  local_irq_save(flags);
  gpio_set_value(pin, 1);
  gpio_set_value(pin, 0);
  local_irq_restore(flags);

  dbg("exit");
}

// interrupt handler
static irq_handler_t 
muon_timer_handler(unsigned irq, void *dev_id, struct pt_regs *regs){
  struct timeval tv;

  dbg("enter");

  // get time of interrupt
  do_gettimeofday(&tv);
  // put it in the fifo
  kfifo_put(&muon_timer_fifo, &tv);
  // FIXME: schedule tasklet/workqueue

  return (irq_handler_t) IRQ_HANDLED;

  dbg("exit");
}

// sysfs entries to control pulse and reset lines
static ssize_t sys_pulse(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
  do_make_pulse(gpio_pulse);
  return 1; // return nonzero or we get called in a loop forever
}

static ssize_t sys_reset(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
  do_make_pulse(gpio_reset);
  return 1;
}

static ssize_t sys_input(struct device *dev, struct device_attribute *attr, char *buf){
  int val;

  val = gpio_get_value(gpio_input);
  return scnprintf(buf, PAGE_SIZE, "%d\n", val);
  
}

//// declare dev_attr_pulse and dev_attr_reset 
static DEVICE_ATTR(pulse, S_IWUSR, NULL, sys_pulse);
static DEVICE_ATTR(reset, S_IWUSR, NULL, sys_reset);
//// declare dev_attr_input
static DEVICE_ATTR(input, S_IRUSR, sys_input, NULL);

// muon_timer_open
static int muon_timer_open(struct inode *inode, struct file *filp){
  int ret;

  dbg("enter");

  //// allow only read access; filched from parrot
  if ( ((filp->f_flags & O_ACCMODE) == O_WRONLY)
       || ((filp->f_flags & O_ACCMODE) == O_RDWR) ) {
    warn("write access is prohibited");
    return -EACCES;
  }
  
  //// grab mutex
  if(!mutex_trylock(&muon_timer_mutex)){
    warn("another process is accessing the muon_timer");
    return -EBUSY;
  }

  //// get gpios
  dbg("reserve gpios");
  ret = gpio_request(gpio_reset, "gpio_reset");
  if( ret ){
    err("unable to reserve gpio_reset: %u %d", gpio_reset, ret);
    goto gpio_reset_fail;
  }
  gpio_direction_output(gpio_reset, 0);
  gpio_export(gpio_reset, false);

  ret = gpio_request(gpio_pulse, "gpio_pulse");
  if( ret ){
    err("unable to reserve gpio_pulse: %u %d", gpio_pulse, ret);
    goto gpio_pulse_fail;
  }
  gpio_direction_output(gpio_pulse, 0);
  gpio_export(gpio_pulse, false);

  ret = gpio_request(gpio_input, "gpio_input");
  if( ret ){
    err("unable to reserve gpio_input: %u %d", gpio_input, ret);
    goto gpio_input_fail;
  }
  gpio_direction_input(gpio_input);
  gpio_export(gpio_input, false);

  dbg("create sysfs files");
  //// setup sysfs controls for reset and pulse and fifo clear
  ret = device_create_file(muon_device, &dev_attr_reset);
  if( ret ){
    err("unable to create sysfs file for reset line");
    goto sysfs_reset_fail;
  } 
  ret = device_create_file(muon_device, &dev_attr_pulse);
  if( ret ){
    err("unable to create sysfs file for pulse line");
    goto sysfs_pulse_fail;
  } 
  ret = device_create_file(muon_device, &dev_attr_input);
  if( ret ){
    err("unable to create sysfs file for input line");
    goto sysfs_input_fail;
  } 

  //// clear fifo
  dbg("reset fifo");
  kfifo_reset(&muon_timer_fifo);

  //// setup interrupts

  dbg("enable interrupt\n");  
  irq = gpio_to_irq(gpio_input);
  ret = request_irq(irq, (irq_handler_t)muon_timer_handler, 
		    IRQF_TRIGGER_RISING, "muon_timer", NULL);
  if( ret ){
    err("failed to reserve irq: %d %d", irq, ret);
    goto request_irq_fail;
  }

  //// do a reset 
  //// FIXME: could check to see if this is needed before doing it....
  dbg("do an initial reset");
  do_make_pulse(gpio_reset);

  dbg("exit");
  return 0;

  // unwind correctly on error
 request_irq_fail:
  device_remove_file(muon_device, &dev_attr_input);
 sysfs_input_fail:
  device_remove_file(muon_device, &dev_attr_pulse);
 sysfs_pulse_fail:
  device_remove_file(muon_device, &dev_attr_reset);
 sysfs_reset_fail:
  gpio_unexport(gpio_input);
  gpio_free(gpio_input);
 gpio_input_fail:
  gpio_unexport(gpio_pulse);
  gpio_free(gpio_pulse);
 gpio_pulse_fail:
  gpio_unexport(gpio_reset);
  gpio_free(gpio_reset);
 gpio_reset_fail:
  mutex_unlock(&muon_timer_mutex);

  dbg("error exit");
  return -EBUSY;
}

// muon_timer_release
static int muon_timer_release(struct inode *inode, struct file *filp){
  dbg("enter");

  //// disable interrupts ... won't be in interrupt context if we get here
  dbg("free irq");
  free_irq(irq, NULL);

  //// free sysfs controls
  dbg("free sysfs files");
  device_remove_file(muon_device, &dev_attr_input);
  device_remove_file(muon_device, &dev_attr_pulse);
  device_remove_file(muon_device, &dev_attr_reset);

  //// release gpios
  dbg("release gpios\n");
  gpio_unexport(gpio_input);
  gpio_free(gpio_input);
  gpio_unexport(gpio_pulse);
  gpio_free(gpio_pulse);
  gpio_unexport(gpio_reset);
  gpio_free(gpio_reset);

  //// release mutex
  mutex_unlock(&muon_timer_mutex);
  dbg("exit");
  return 0;
}
// muon_timer_read
//// pull from fifo, "translate", and return

static struct file_operations fops = {
  //  .read = muon_timer_read,
    .open = muon_timer_open,
    .release = muon_timer_release
};

// device setup and teardown
//// FIXME: ought to test pin numbers to make sure they're sane... gpio_is_valid
static int __init muon_timer_init(void){
  int retval = 0;

  dbg("enter");
  // Register char device and get a muon_major number
  muon_major = register_chrdev(0, DEVICE_NAME, &fops);
  if( muon_major<0 ){
    err("failed to register char device: error %d", muon_major);
    retval = muon_major;
    goto failed_chrdev;
  }

  // Register the muon_timer virtual class ... no physical bus to
  // connect to
  muon_class = class_create(THIS_MODULE, CLASS_NAME);
  if(IS_ERR(muon_class)){
    err("failed to register device class '%s'", CLASS_NAME);
    retval = PTR_ERR(muon_class);
    goto failed_class;
  }

  // create device and nodes in /dev
  muon_device = device_create(muon_class, NULL, MKDEV(muon_major, 0),
			      NULL, CLASS_NAME);
  if(IS_ERR(muon_device)){
    err("failed to create device '%s'", CLASS_NAME);
    retval = PTR_ERR(muon_device);
    goto failed_create;
  }

  return 0;

 failed_create:
  class_destroy(muon_class);
 failed_class:
  unregister_chrdev(muon_major, DEVICE_NAME);
 failed_chrdev:

  dbg("exit");
  return retval;
}

static void __exit muon_timer_exit(void){
  struct timeval tv;

  dbg("enter");

  while( kfifo_get(&muon_timer_fifo, &tv))
    info("from muon_timer_fifo: %ld %ld", tv.tv_sec, tv.tv_usec);

  // at this point, no other thread should exist with access to
  // muon_timer_fifo, so reset it before we go away
  kfifo_reset(&muon_timer_fifo);

  device_destroy(muon_class, MKDEV(muon_major,0));
  class_destroy(muon_class);
  unregister_chrdev(muon_major, DEVICE_NAME);

  dbg("exit");
}


module_init(muon_timer_init);
module_exit(muon_timer_exit);

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/time.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Lynch");
MODULE_DESCRIPTION("An interrupt-based timing measurement driver for the BBB");
MODULE_VERSION("1.0");

// Macros swiped from parrot driver, with a few tweaks for consistency
#define dbg(format, arg...) do { if (debug) pr_info(CLASS_NAME ": %s: " format "\n", __FUNCTION__ , ## arg); } while (0)
#define err(format, arg...) do { pr_err(CLASS_NAME ": %s: " format "\n", __FUNCTION__ , ## arg); } while (0)
#define info(format, arg...) do { pr_info(CLASS_NAME ": %s: " format "\n", __FUNCTION__ , ## arg); } while (0)
#define warn(format, arg...) do { pr_warn(CLASS_NAME ": %s: " format "\n", __FUNCTION__ , ## arg); } while (0)


#define DEVICE_NAME_BASE "muon_timer"
#define CLASS_NAME "muon_timer"
#define MUON_TIMER_FIFO_SIZE 1024

// module parameters appear in /sys/module/muon_timer/parameters
// add 'debug=1' on insmod to change .. 
///
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
/// muon timer timeout in seconds
static int timer_timeout = 5;
module_param(timer_timeout, uint, S_IRUGO);
MODULE_PARM_DESC(timer_timeout, "Timeout in seconds for interrupt loss monitor timer in seconds (default: 5)");

static int muon_major;
static struct class *muon_class = 0;
static struct device *muon_device = 0;
static int irq;
static int timeout_jiffies;
// for accounting data
static int cur_ints, cur_missed, tot_ints, tot_missed;

// We'll allow only one process into the timer at once
static DEFINE_MUTEX(muon_timer_mutex);

// define fifo ... 
static DEFINE_KFIFO(muon_timer_fifo, struct timespec, MUON_TIMER_FIFO_SIZE);

// make pulse
static void do_make_pulse(unsigned pin){
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

// device read wait queue
static DECLARE_WAIT_QUEUE_HEAD(read_queue);

// device timer
static void do_timer_expired(unsigned long u);
static struct timer_list timer = TIMER_INITIALIZER(do_timer_expired, 0, 0);

// timer function
static void do_timer_expired(unsigned long u){
  dbg("enter");
  warn("timer expired!");
  if( gpio_get_value(gpio_input) ){
    err("missed an interrupt!  Input high ... resetting");
    do_make_pulse(gpio_reset);
    ++cur_missed;
    ++tot_missed;
  }
  dbg("reset timer");
  mod_timer(&timer, jiffies+timeout_jiffies);
  dbg("exit");
}

// reset/wakeup tasklet
static void do_reset_tasklet(unsigned long data){
  dbg("enter");
  dbg("make reset pulse");
  do_make_pulse(gpio_reset);
  dbg("wake_up");
  wake_up_interruptible(&read_queue);
  dbg("reset timer");
  mod_timer(&timer, jiffies+timeout_jiffies);
  dbg("exit");
}

static DECLARE_TASKLET(muon_reset_tasklet, do_reset_tasklet, 0);

// going to need a recovery timer, too

// interrupt handler
static irq_handler_t 
muon_timer_handler(unsigned irq, void *dev_id, struct pt_regs *regs){
  struct timespec ts;

  dbg("enter");

  // get time of interrupt
  //  do_gettimeofday(&ts);
  getnstimeofday(&ts);
  // put it in the fifo
  kfifo_put(&muon_timer_fifo, &ts);
  // schedule reset tasklet
  tasklet_schedule(&muon_reset_tasklet);
  // accounting
  ++cur_ints;
  ++tot_ints;

  dbg("interrupt: %ld %ld", ts.tv_sec, ts.tv_nsec);

  dbg("exit");
  return (irq_handler_t) IRQ_HANDLED;

}

// sysfs entries to control pulse and reset lines
static ssize_t sys_pulse(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
  dbg("enter");
  do_make_pulse(gpio_pulse);
  dbg("exit");
  return count;
}

static ssize_t sys_reset(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
  dbg("enter");
  do_make_pulse(gpio_reset);
  dbg("exit");
  return count;
}

static ssize_t sys_input(struct device *dev, struct device_attribute *attr, char *buf){
  int val;

  dbg("enter");
  val = gpio_get_value(gpio_input);
  dbg("exit");
  return scnprintf(buf, PAGE_SIZE, "%d\n", val);
  
}

// sysfs entries for accounting
static ssize_t sys_print_int(struct device *dev, struct device_attribute *attr, char *buf, int val){
  dbg("enter");
  dbg("exit");
  return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}
static ssize_t sys_cur_ints(struct device *dev, struct device_attribute *attr, char *buf){
  dbg("enter");
  dbg("exit");
  return sys_print_int(dev, attr, buf, cur_ints);
}
static ssize_t sys_cur_missed(struct device *dev, struct device_attribute *attr, char *buf){
  dbg("enter");
  dbg("exit");
  return sys_print_int(dev, attr, buf, cur_missed);
}
static ssize_t sys_tot_ints(struct device *dev, struct device_attribute *attr, char *buf){
  dbg("enter");
  dbg("exit");
  return sys_print_int(dev, attr, buf, tot_ints);
}
static ssize_t sys_tot_missed(struct device *dev, struct device_attribute *attr, char *buf){
  dbg("enter");
  dbg("exit");
  return sys_print_int(dev, attr, buf, tot_missed);
}

//// declare dev_attr_pulse and dev_attr_reset 
///// these should be S_IWUSR | S_IWGRP for group muons, but I can't seem to convince udev to make the appropriate changes
static DEVICE_ATTR(pulse, S_IWUGO, NULL, sys_pulse);
static DEVICE_ATTR(reset, S_IWUGO, NULL, sys_reset);
//// declare dev_attr_input
static DEVICE_ATTR(input, S_IRUGO, sys_input, NULL);
static DEVICE_ATTR(current_interrupts, S_IRUGO, sys_cur_ints, NULL);
static DEVICE_ATTR(current_missed, S_IRUGO, sys_cur_missed, NULL);
static DEVICE_ATTR(total_interrupts, S_IRUGO, sys_tot_ints, NULL);
static DEVICE_ATTR(total_missed, S_IRUGO, sys_tot_missed, NULL);

// This buffer is used in the read functions to do transfers to userspace
#define BUFSIZE 4096
static char buffer[BUFSIZE];
static char *writep, *readp;

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

  //// reset accounting
  dbg("resetting accounting data");
  cur_ints = cur_missed = 0;

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

  //// clear transfer buffer
  dbg("clear transfer buffer");
  writep = readp = buffer;

  //// setup the timer
  dbg("enable timer");
  timer.expires = jiffies + timeout_jiffies;
  add_timer(&timer);

  //// setup interrupts
  dbg("enable interrupt");  
  irq = gpio_to_irq(gpio_input);
  ret = request_irq(irq, (irq_handler_t)muon_timer_handler, 
		    IRQF_TRIGGER_RISING, "muon_timer", NULL);
  if( ret ){
    err("failed to reserve irq: %d %d", irq, ret);
    goto request_irq_fail;
  }

  //// do a reset 
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

  //// remove timer
  dbg("remove timer");
  del_timer_sync(&timer);

  //// free sysfs controls
  dbg("free sysfs files");
  device_remove_file(muon_device, &dev_attr_input);
  device_remove_file(muon_device, &dev_attr_pulse);
  device_remove_file(muon_device, &dev_attr_reset);

  //// release gpios
  dbg("release gpios");
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

static int has_data(void) {
  return (writep>readp || !kfifo_is_empty(&muon_timer_fifo));
}

// muon_timer_binary_read
static ssize_t muon_timer_binary_read(struct file *filp, char __user *buf, 
			       size_t count, loff_t *f_pos){
  dbg("enter");

  dbg("has_data %d", has_data());

  // if it's empty ...
  while( !has_data() ){
    // ... return immediately if non-blocking
    dbg("check if nonblocking");
    if(filp->f_flags & O_NONBLOCK){
      dbg("nonblocking exit");
      return -EAGAIN;
    }
    // ... otherwise, put process to sleep
    dbg("going to wait");
    if( wait_event_interruptible(read_queue, has_data()) ){
      // have filesystem core restart the read
      dbg("done waiting with signal");
      return -ERESTARTSYS;
    }
    dbg("done waiting");
  }

  // If we get here, we have data to return, either already in buffer,
  // or in fifo
  /// refill transfer buffer if necessary
  dbg("check transfer buffer");
  if( writep==readp ){
    //// reset to start of buffer
    dbg("buffer exhausted: reset");
    writep = readp = buffer;
    //// fifo can't be empty here .... drain as much as possible
    dbg("drain fifo");
    while( !kfifo_is_empty(&muon_timer_fifo) ){
      ///// if a write to the buffer will overrun end, stop draining
      dbg("fifo not empty yet");
      if( writep + sizeof(struct timespec) - buffer >= BUFSIZE ){
	dbg("get would overflow buffer");
	break; 
      }
      ///// otherwise, space available and data to move
      dbg("fifo_get");
      if( !kfifo_get(&muon_timer_fifo, (struct timespec*)writep) ){
	///// fifo can't be empty ... silencing compiler warning
	err("kfifo_get failed!");
	return -EFAULT;
      }
      writep+=sizeof(struct timespec);

    }
  }
  // here, we have data in transfer buffer, transfer it to userspace
  dbg("return data to userspace");
  dbg("count requested: %d", count);
  dbg("count available: %d", (size_t)(writep-readp));
  count = min(count, (size_t)(writep-readp)); // don't try too hard
  dbg("count to return: %d", count);
  if( copy_to_user(buf, readp, count) ){
    /// something went wrong ...
    err("something went wrong in copy_to_user");
    return -EFAULT;
  }
  readp+=count;
  *f_pos += count;

  dbg("exit");

  return count;  
}

static struct file_operations binary_fops = {
  .owner = THIS_MODULE,
  .read = muon_timer_binary_read,
  .open = muon_timer_open,
  .release = muon_timer_release
};

// device setup and teardown
static int __init muon_timer_init(void){
  int retval = 0;

  dbg("enter");

  //  dbg("sizeof struct timespec %d", sizeof(struct timespec));
  timeout_jiffies = timer_timeout*HZ;
  dbg("timeout_jiffies: %d", HZ);

  // test pin numbers before doing any actual kernel magic
  dbg("test gpio pins for validity");
  if( !gpio_is_valid(gpio_pulse) ){
    err("Invalid gpio pin for pulse: %d", gpio_pulse);
    return -EINVAL;
  }
  if( !gpio_is_valid(gpio_reset) ){
    err("Invalid gpio pin for reset: %d", gpio_reset);
    return -EINVAL;
  }
  if( !gpio_is_valid(gpio_input) ){
    err("Invalid gpio pin for pulse: %d", gpio_input);
    return -EINVAL;
  }

  // Register the muon_timer virtual class ... no physical bus to
  // connect to
  dbg("register device class");
  muon_class = class_create(THIS_MODULE, CLASS_NAME);
  if(IS_ERR(muon_class)){
    err("failed to register device class '%s'", CLASS_NAME);
    retval = PTR_ERR(muon_class);
    goto failed_class;
  }

  // Register char device and get a muon_major number
  dbg("register binary char dev and get major number");
  muon_major = register_chrdev(0, DEVICE_NAME_BASE, &binary_fops);
  if( muon_major<0 ){
    err("failed to register char device: error %d", muon_major);
    retval = muon_major;
    goto failed_chardev;
  }

  // create device and nodes in /dev
  dbg("create device muon_timer");
  muon_device = device_create(muon_class, NULL, MKDEV(muon_major, 0),
			      NULL, DEVICE_NAME_BASE);
  if(IS_ERR(muon_device)){
    err("failed to create device '%s'", DEVICE_NAME_BASE);
    retval = PTR_ERR(muon_device);
    goto failed_dev;
  }

  // sysfs nodes
  dbg("create sysfs nodes");
  retval = device_create_file(muon_device, &dev_attr_current_interrupts);
  if( retval ){
    err("unable to create sysfs file for current interrupt accounting");
    goto failed_sysfs_cur_ints;
  } 
  retval = device_create_file(muon_device, &dev_attr_current_missed);
  if( retval ){
    err("unable to create sysfs file for current missed interrupt accounting");
    goto failed_sysfs_cur_missed;
  } 
  retval = device_create_file(muon_device, &dev_attr_total_interrupts);
  if( retval ){
    err("unable to create sysfs file for total interrupt accounting");
    goto failed_sysfs_tot_ints;
  } 
  retval = device_create_file(muon_device, &dev_attr_total_missed);
  if( retval ){
    err("unable to create sysfs file for total missed interrupt accounting");
    goto failed_sysfs_tot_missed;
  } 

  tot_ints = tot_missed = 0;

  dbg("exit");
  return 0;

 failed_sysfs_tot_missed:
  device_remove_file(muon_device, &dev_attr_total_interrupts);
 failed_sysfs_tot_ints:
  device_remove_file(muon_device, &dev_attr_current_missed);
 failed_sysfs_cur_missed:
  device_remove_file(muon_device, &dev_attr_current_interrupts);
 failed_sysfs_cur_ints:
  device_destroy(muon_class, MKDEV(muon_major,0));
 failed_dev:
  unregister_chrdev(muon_major, DEVICE_NAME_BASE);
 failed_chardev:
  class_destroy(muon_class);
 failed_class:

  dbg("error exit");
  return retval;
}

static void __exit muon_timer_exit(void){
  dbg("enter");

  // at this point, no other thread should exist with access to
  // muon_timer_fifo, so reset it before we go away
  kfifo_reset(&muon_timer_fifo);

  device_remove_file(muon_device, &dev_attr_total_missed);
  device_remove_file(muon_device, &dev_attr_total_interrupts);
  device_remove_file(muon_device, &dev_attr_current_missed);
  device_remove_file(muon_device, &dev_attr_current_interrupts);

  device_destroy(muon_class, MKDEV(muon_major,0));
  class_destroy(muon_class);
  unregister_chrdev(muon_major, DEVICE_NAME_BASE);

  dbg("exit");
}


module_init(muon_timer_init);
module_exit(muon_timer_exit);

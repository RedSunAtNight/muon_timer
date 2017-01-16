#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/time.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Lynch");
MODULE_DESCRIPTION("Test driver to figure out why eof isn't working");
MODULE_VERSION("1.0");

#define CLASS_NAME "eof_class"

// Macros swiped from parrot driver, with a few tweaks for consistency
#define dbg(format, arg...) do { if (debug) pr_info(CLASS_NAME ": %s: " format "\n", __FUNCTION__ , ## arg); } while (0)
#define err(format, arg...) do { pr_err(CLASS_NAME ": %s: " format "\n", __FUNCTION__ , ## arg); } while (0)
#define info(format, arg...) do { pr_info(CLASS_NAME ": %s: " format "\n", __FUNCTION__ , ## arg); } while (0)
#define warn(format, arg...) do { pr_warn(CLASS_NAME ": %s: " format "\n", __FUNCTION__ , ## arg); } while (0)

static bool debug = true;

static int eof_major;
static struct class *eof_class = 0;
static struct device *eof_device = 0;

#define BUFFERS 8
#define BUFSIZE 256

char buffers[BUFFERS][BUFSIZE];

void fill_text(void){
  int i;
  char *buf[] = {"a\n", "bb\n", "ccc\n", "dddd\n", "eeeee\n",
		 "ffffff\n", "ggggggg\n", "hhhhhhhh\n"};
  for(i=0; i<BUFFERS; ++i){
    strncpy(buffers[i], buf[i], strlen(buf[i])+1);
  }
}

void fill_binary(void){
  int i, j;
  for(i=0; i<BUFFERS; ++i){
    for(j=0; j<i; ++j){
      buffers[i][j] = (char)0xff;
    }
    buffers[i][i] = '\0';
  }
}

int i = 0;

static ssize_t eof_read(struct file *filp, char __user *buf, 
			size_t count, loff_t *f_pos){
  int cnt = 0;
  dbg("enter: %d", BUFFERS);
  if( i>= BUFFERS )
    return 0;
  cnt = strlen(buffers[i]) +1 ;
  dbg("count = %d", cnt);
  if( copy_to_user(buf, buffers[i], cnt)){
    err("it blowed up!");
    return -EFAULT;
  }
  *f_pos += cnt;
  ++i;
  dbg("exit");
  return cnt;
}

static int eof_open(struct inode *inode, struct file *filp){
  dbg("enter");
  //  fill_text();
  fill_binary();
  dbg("exit");
  return 0;
}

static int eof_release(struct inode *inode, struct file *filp){
  dbg("enter");
  dbg("exit");
  return 0;
}

static struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = eof_read,
  .open = eof_open,
  .release = eof_release
};

static int __init eof_test_init(void){
  int retval = 0;

  dbg("enter");
 
  eof_major = register_chrdev(0, "eof_test", &fops);
  if( eof_major < 0 ){
    err("failed to register eof_major");
    retval = eof_major;
    goto failed_chrdev;
  }

  eof_class = class_create(THIS_MODULE, "eof");
  if( IS_ERR(eof_class) ){
    err("failed to register device class");
    retval = PTR_ERR(eof_class);
    goto failed_class;
  }

  eof_device = device_create(eof_class, NULL, MKDEV(eof_major, 0),
			     NULL, "eof_test");
  if( IS_ERR(eof_device) ){
    err("failed to create device");
    retval = PTR_ERR(eof_device);
    goto failed_create;
  }

  dbg("exit");
  return 0;
  
 failed_create:
  class_destroy(eof_class);
 failed_class:
  unregister_chrdev(eof_major, "eof_test");
 failed_chrdev:

  dbg("erro exit");
  return retval;

}

static void __exit eof_test_exit(void){
  dbg("enter");
  device_destroy(eof_class, MKDEV(eof_major,0));
  class_destroy(eof_class);
  unregister_chrdev(eof_major, "eof_test");
  dbg("exit");
}

module_init(eof_test_init);
module_exit(eof_test_exit);

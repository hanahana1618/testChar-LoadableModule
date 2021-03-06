//CHANGES TO IMPLEMENT
 /*
 * file testchar.c
 * Sample LKM for Embedded OS at FIU
 */
 
#include "testchar.h"

static struct file_operations fops = {
   .owner = THIS_MODULE,
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release
};
 
static int __init testchar_init(void) { 
	//save major number
   major = register_chrdev(0, DEVICE_NAME, &fops);
   if (major<0){
      printk(KERN_ALERT "TestChar failed to register major number\n");
      return major; 
   }
   printk(KERN_INFO "TestChar: registered a major number %d\n", major);
 
   // Register the device class
   testcharClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(testcharClass)) {           
      unregister_chrdev(major, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class. Exiting init\n");
      return PTR_ERR(testcharClass);        
   }
   printk(KERN_INFO "TestChar: device class registered correctly\n");
 
   // Register the device driver
   testcharDevice = device_create(testcharClass, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
   if (IS_ERR(testcharDevice)) {     //unregister if error
      class_destroy(testcharClass);          
      unregister_chrdev(major, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device class\n");
      return PTR_ERR(testcharDevice);
   }
   printk(KERN_INFO "TestChar: device class created correctly\n"); 

   //lock mutex when the device driver is init
   mutex_init(&testcharMutex);

   return 0;
}
 
static int dev_open(struct inode *inodep, struct file *filep){
	//book code: struct dev

   //mutex_init(&testcharMutex);

   if (!mutex_trylock(&testcharMutex)) {
      //print this to user space, not kernel log
      printk("Cannot Access the Device Driver TestChar because another process is using it.\n");
      //return something here
      return -EBUSY;
   }

   printk(KERN_INFO "TestChar: Device has been successfully opened.\n");
   return 0;
}
 
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int errorCount = 0;
   errorCount = copy_to_user(buffer, message, sizeMssg);
 
   if (errorCount==0){           
      printk(KERN_INFO "TestChar: Sent %d characters to the user\n", sizeMssg);
      return (sizeMssg=0); 
   }
   else {
      printk(KERN_INFO "TestChar: Failed to send %d characters to the user\n", errorCount);
      return -EFAULT;            
   }
}
 
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
   sprintf(message, "%s(%zu letters)", buffer, len);   
   sizeMssg = strlen(message);  

   //debugging for the write function             
   printk(KERN_INFO "TestChar: Received %zu characters from the user\n", len);
   return len;
}
 
static int dev_release(struct inode *inodep, struct file *filep) {
	//no hardware to shut down

   //unlock the mutex so that another use can have it
   mutex_unlock(&testcharMutex);
   //mutex_destroy(&testcharMutex);

   //debugging for make-believe hardware and mutex unlocked
   printk(KERN_INFO "TestChar: Device has been successfully closed\n");
   return 0;
}

static void __exit testchar_exit(void) {
   //destroy the mutex so that the next user can have it
   mutex_destroy(&testcharMutex);

   //make sure to sign out of the device driver
   device_destroy(testcharClass, MKDEV(major, 0));    
   class_unregister(testcharClass);                         
   class_destroy(testcharClass);                    
   unregister_chrdev(major, DEVICE_NAME);     

   //print out debug message on the kernel log
   printk(KERN_INFO "TestChar: Goodbye from the LKM!\n");
}
 
module_init(testchar_init);
module_exit(testchar_exit);

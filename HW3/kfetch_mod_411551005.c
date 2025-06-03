#include <asm/errno.h>
#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h> 
#include <linux/ktime.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/sched.h> /* for_each_process */
#include <linux/sched/signal.h>
#include <linux/smp.h>
#include <linux/types.h>
#include <linux/uaccess.h> /* for get_user and put_user */
#include <linux/utsname.h> // for utsname() -> release, nodename
#include <linux/sysinfo.h>  
#include <linux/version.h>

#define SUCCESS 0
#define DEVICE_NAME "kfetch" /* Dev name as it appears in /proc/devices   */
#define KFETCH_BUF_SIZE 8000 /* Max length of the message from the device */

#define KFETCH_NUM_INFO 6
#define KFETCH_RELEASE (1 << 0)
#define KFETCH_NUM_CPUS (1 << 1)
#define KFETCH_CPU_MODEL (1 << 2)
#define KFETCH_MEM (1 << 3)
#define KFETCH_UPTIME (1 << 4)
#define KFETCH_NUM_PROCS (1 << 5)
 
static int kfetch_open(struct inode *inode, struct file *file); 
static int kfetch_release(struct inode *inode, struct file *file); 
static ssize_t kfetch_read(struct file *file, char __user *buf, size_t count, loff_t *ppos); 
static ssize_t kfetch_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos); 

enum { 
    KFETCH_NOT_USED = 0, 
    KFETCH_EXCLUSIVE_OPEN = 1, 
}; 

static atomic_t already_open = ATOMIC_INIT(KFETCH_NOT_USED); 
static char msg[KFETCH_BUF_SIZE]; 
static struct class *cls; 
static int kfetch_mask = 0;
static struct class *cls; 
static int major;
unsigned int mask = 127;
const char *asciilogo[] = {
        "888b    888 Y88b   d88P  .d8888b.  888     888      .d88888b.   .d8888b.   .d8888b. ",
        "8888b   888  Y88b d88P  d88P  Y88b 888     888     d88P' 'Y88b d88P  Y88b d88P  Y88b",
        "88888b  888   Y88o88P   888    888 888     888     888     888 Y88b.      888    888",
        "888Y88b 888    Y888P    888        888     888     888     888  'Y888b.   888       ",
        "888 Y88b888     888     888        888     888     888     888     'Y88b. 888       ",
        "888  Y88888     888     888    888 888     888     888     888       '888 888    888",
        "888   Y8888     888     Y88b  d88P Y88b. .d88P     Y88b. .d88P Y88b  d88P Y88b  d88P",
        "888    Y888     888      'Y8888P'   'Y88888P'       'Y88888P'   'Y8888P'   'Y8888P' "
    };

const static struct file_operations kfetch_ops = {
    .owner   = THIS_MODULE,
    .read    = kfetch_read,
    .write   = kfetch_write,
    .open    = kfetch_open,
    .release = kfetch_release,
};

static int kfetch_init(void){ 
    major = register_chrdev(0, DEVICE_NAME, &kfetch_ops); 
    if (major < 0) { 
        pr_alert("Registering char device failed with %d\n", major); 
        return major; 
    } 
    pr_info("I was assigned major number %d.\n", major); 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0) 
    cls = class_create(DEVICE_NAME); 
#else 
    cls = class_create(THIS_MODULE, DEVICE_NAME); 
#endif 
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME); 
    pr_info("Device created on /dev/%s\n", DEVICE_NAME); 
    kfetch_mask = ((1 << KFETCH_NUM_INFO) - 1);

    return SUCCESS; 
} 

static void kfetch_exit(void){ 
    device_destroy(cls, MKDEV(major, 0)); 
    class_destroy(cls); 
    /* Unregister the device */ 
    unregister_chrdev(major, DEVICE_NAME); 
} 

static int kfetch_open(struct inode *inode, struct file *file){ 
    if (atomic_cmpxchg(&already_open, KFETCH_NOT_USED, KFETCH_EXCLUSIVE_OPEN)) 
        return -EBUSY; 
    try_module_get(THIS_MODULE); 
    return SUCCESS; 
} 

static int kfetch_release(struct inode *inode, struct file *file){ 
    atomic_set(&already_open, KFETCH_NOT_USED); 
    module_put(THIS_MODULE); 
    return SUCCESS; 
} 

static ssize_t kfetch_read(struct file *filp,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset){

    int bytes_buffer = 0, index = 0;


    /* fetching the information */
    char *kernel_name;
    char dashline[50];
    
    int num_procs = 0, nodename_len = 0;
    
    struct new_utsname *uts = utsname();
    struct sysinfo si;
    struct cpuinfo_x86 *c = &cpu_data(0);
    struct task_struct *task_list;
    s64 uptime;

    si_meminfo(&si);
    
    kernel_name = uts->release;
    nodename_len = strlen(uts->nodename);
    for (int i = 0; i < nodename_len; i++) dashline[i] = '-';
    dashline[nodename_len] = '\0';

    for_each_process(task_list) {
        num_procs++;
    }
    uptime = ktime_divns(ktime_get_coarse_boottime(), NSEC_PER_SEC)/  60;
    /* end of fetching the information */
    
    /*print first line asciilogo[0],  +  host name*/
    bytes_buffer += sprintf(msg + bytes_buffer, "%s\t\033[1;33m%s\033[0m\n", asciilogo[index], uts->nodename);
    index += 1;

    /*print second line asciilogo[1],  +  dash line*/
    bytes_buffer += sprintf(msg + bytes_buffer, "%s\t%s\n", asciilogo[index], dashline);
    index += 1;

    
    /*for kernel mask part*/
    if(mask & KFETCH_RELEASE){
        bytes_buffer += sprintf(msg + bytes_buffer, "%s\t\033[1;33mKernel:\033[0m\t%s\n", asciilogo[index], kernel_name);
        index += 1;
        printk("KFETCH_RELEASE");
    }
    if(mask & KFETCH_CPU_MODEL){
        bytes_buffer += sprintf(msg + bytes_buffer, "%s\t\033[1;33mCPU:\033[0m\t%s\n", asciilogo[index], c->x86_model_id);
        index += 1;
        printk("KFETCH_CPU_MODEL");
    }
    if(mask & KFETCH_NUM_CPUS){
        bytes_buffer += sprintf(msg + bytes_buffer, "%s\t\033[1;33mCPUs:\033[0m\t%d / %d\n", asciilogo[index], num_online_cpus(), num_active_cpus());
        index += 1;
        printk("KFETCH_NUM_CPUS");
    }
    if(mask & KFETCH_MEM){
        bytes_buffer += sprintf(msg + bytes_buffer, "%s\t\033[1;33mMem:\033[0m\t%ld MB / %ld MB\n", asciilogo[index], si.freeram * 4 / 1024, si.totalram * 4 / 1024);
        index += 1;
        printk("KFETCH_MEM");
    }
    if(mask & KFETCH_NUM_PROCS){
        bytes_buffer += sprintf(msg + bytes_buffer, "%s\t\033[1;33mProc:\033[0m\t%d\n", asciilogo[index], num_procs);
        index += 1;
        printk("KFETCH_NUM_PROCS");
    }
    if(mask & KFETCH_UPTIME){
        bytes_buffer += sprintf(msg + bytes_buffer, "%s\t\033[1;33mUptime:\033[0m\t%lld mins\n", asciilogo[index], uptime);
        index += 1;
        printk("KFETCH_UPTIME");
    }
    /*if no full mask and print the rest part of asciilogo*/
    while (index < 8){
        bytes_buffer += sprintf(msg + bytes_buffer, "%s\n", asciilogo[index]);
        index += 1;
        printk("EMPTY");
    }

    if (copy_to_user(buffer, msg, bytes_buffer)) {
		pr_alert("Failed to copy data to user");
		return 0;
	}
    /* cleaning up */
    return bytes_buffer;
}

/* Called when a process writes to dev file; echo "enable" > /dev/key_state */ 
static ssize_t kfetch_write(struct file *filp,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset){
    int mask_info; 
	if (copy_from_user(&mask_info, buffer, length)) {
		pr_alert("Failed to copy data from user");
		return 0;
	}
	mask = mask_info;
	return 0;
}

module_init(kfetch_init); 
module_exit(kfetch_exit); 

MODULE_AUTHOR("411551005");
MODULE_LICENSE("GPL");

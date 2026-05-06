#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");

static spinlock_t lock_a = __SPIN_LOCK_UNLOCKED(lock_a);
static spinlock_t lock_b = __SPIN_LOCK_UNLOCKED(lock_b);

static struct task_struct *thread1;
static struct task_struct *thread2;

/* Thread 1: Acquire lock_a first, then lock_b */
static int thread_func_1(void *data)
{
    spin_lock(&lock_a);
    printk(KERN_INFO "Thread 1: Acquired lock_a\n");
    
    msleep(100);
    
    printk(KERN_INFO "Thread 1: Trying to acquire lock_b...\n");
    spin_lock(&lock_b);
    printk(KERN_INFO "Thread 1: Acquired lock_b\n");
    
    spin_unlock(&lock_b);
    spin_unlock(&lock_a);
    
    return 0;
}

/* Thread 2: Acquire lock_b first, then lock_a */
static int thread_func_2(void *data)
{
    spin_lock(&lock_b);
    printk(KERN_INFO "Thread 2: Acquired lock_b\n");
    
    msleep(50);
    
    printk(KERN_INFO "Thread 2: Trying to acquire lock_a...\n");
    spin_lock(&lock_a);
    printk(KERN_INFO "Thread 2: Acquired lock_a\n");
    
    spin_unlock(&lock_a);
    spin_unlock(&lock_b);
    
    return 0;
}

static int __init deadlock_init(void)
{
    printk(KERN_INFO "\n=== Deadlock Test Module ===\n");
    printk(KERN_INFO "Creating threads...\n");
    
    thread1 = kthread_create(thread_func_1, NULL, "thread_1");
    thread2 = kthread_create(thread_func_2, NULL, "thread_2");
    
    if (thread1 && thread2) {
        wake_up_process(thread1);
        wake_up_process(thread2);
        printk(KERN_INFO "Threads started - Deadlock will occur\n");
    }
    
    return 0;
}

static void __exit deadlock_exit(void)
{
    printk(KERN_INFO "Module unloading\n");
    
    if (thread1)
        kthread_stop(thread1);
    if (thread2)
        kthread_stop(thread2);
}

module_init(deadlock_init);
module_exit(deadlock_exit);

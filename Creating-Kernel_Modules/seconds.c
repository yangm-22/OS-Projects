/**
 * 
 * File Name: seconds.c
 * Purpose: Kernel module that communicates with /proc file system and reports
the number of elapsed seconds since the kernel module was loaded.
 * 
 * 
 * Linux Distribution Information:
 * Distributor ID: Ubuntu
 * Description:	Ubuntu 25.10
 * Release: 25.10
 * Codename: questing
 * Kernel Version: 6.17.0-8-generic
 * 
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h> 

#define BUFFER_SIZE 128
#define PROC_NAME "seconds"

/* Function prototypes */
ssize_t proc_read(struct file *file, char *buf, size_t count, loff_t *pos);
int proc_init(void);
void proc_exit(void);

/* define structure of proc_ops */
static const struct proc_ops my_proc_ops = {
        .proc_read = proc_read,
};

/* global variable(s) */
unsigned long jiffies_start; 

/* This function is called when the module is loaded. */
int proc_init(void)
{
        // store jiffies_start when module is first loaded
        jiffies_start = jiffies;

        // creates the /proc/seconds entry
        // the following function call is a wrapper for
        // proc_create_data() passing NULL as the last argument
        proc_create(PROC_NAME, 0, NULL, &my_proc_ops);

        printk(KERN_INFO "/proc/%s created\n", PROC_NAME);

	return 0;
}

/* This function is called when the module is removed. */
void proc_exit(void) {

        // removes the /proc/seconds entry
        remove_proc_entry(PROC_NAME, NULL);

        printk( KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

/**
 * This function is called each time the /proc/seconds is read.
 * 
 * This function is called repeatedly until it returns 0, so
 * there must be logic that ensures it ultimately returns 0
 * once it has collected the data that is to go into the 
 * corresponding /proc file.
 *
 * Input Params:
 * file: pointer to file struct of /proc/seconds
 * usr_buf: stores copied the contents of kernel memory buffer (in user space) 
 * count: size of the user buffer
 * pos: pointer to file position
 * 
 * Output/Returns: 
 * number of bytes written to user buffer or 0
 */
ssize_t proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos)
{
        int rv = 0; // number of bytes written to user buffer
        char buffer[BUFFER_SIZE]; // kernel buffer to store output text 
        static int completed = 0; // flag to confirm output is returned only once
        unsigned long jiffies_elapsed; // number of jiffies elapsed since module loaded
        unsigned long seconds_elapsed; // number of seconds elapsed since module loaded

        // if function already called => return 0
        if (completed) {
                completed = 0; // reset flag for next read
                return 0;
        }

        completed = 1;

        // determine elapsed time in jiffies and convert to seconds
        jiffies_elapsed = jiffies - jiffies_start;
        seconds_elapsed = jiffies_elapsed/HZ;

        rv = sprintf(buffer, "%lu \n", seconds_elapsed);

        // copies the contents of buffer to userspace usr_buf
        copy_to_user(usr_buf, buffer, rv);

        return rv; 
}


/* Macros for registering module entry and exit points. */
module_init( proc_init );
module_exit( proc_exit );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hello Module");
MODULE_AUTHOR("SGG");


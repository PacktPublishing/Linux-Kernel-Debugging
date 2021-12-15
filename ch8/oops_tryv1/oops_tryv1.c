/*
 * ch7/oops_tryv1/oops_tryv1.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Debugging"
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Debugging
 *
 * From: Ch 7: Oops! Interpreting the kernel bug diagnostic
 ****************************************************************
 * Brief Description:
 *
 * For details, please refer the book, Ch 7.
 */
#define pr_fmt(fmt) "%s:%s():%d: " fmt, KBUILD_MODNAME, __func__, __LINE__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_AUTHOR("<insert your name here>");
MODULE_DESCRIPTION("LKD book:ch7/oops_tryv1: generates a kernel Oops! a kernel bug");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

static bool try_reading;
module_param(try_reading, bool, 0644);
MODULE_PARM_DESC(try_reading,
"Trigger an Oops-generating bug when reading from NULL; else, do so by writing to NULL");

static int __init try_oops_init(void)
{
	size_t val = 0x0;

	pr_info("Lets Oops!\nNow attempting to %s something %s the NULL address 0x%p\n",
		!!try_reading ? "read" : "write", 
		!!try_reading ? "from" : "to",  // pedantic, huh
		NULL);
	if (!!try_reading) {
		val = *(int *)0x0;
		/* Interesting! If we leave the code at this, the compiler actually optimizes
		 * it away, as we're not working with the result of the read. This makes it
		 * appear that the read does NOT cause an Oops; this ISN'T the case, it does,
		 * of course.
		 * So, to prove it, we try and printk the variable, thus forcing the compiler
		 * to generate the code, and voila, we're rewarded with a nice Oops !
		 */
		pr_info("val = 0x%lx\n", val);
	} else // try writing to NULL
		*(int *)val = 'x';

	return 0;		/* success */
}

static void __exit try_oops_exit(void)
{
	pr_info("Goodbye, from Oops try v1\n");
}

module_init(try_oops_init);
module_exit(try_oops_exit);

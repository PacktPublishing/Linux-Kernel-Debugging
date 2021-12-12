/*
 * ch8/oops_try.c
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
 * For details, please refer the book, Ch 8.
 */
#define pr_fmt(fmt) "%s:%s():%d: " fmt, KBUILD_MODNAME, __func__, __LINE__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/random.h>

MODULE_AUTHOR("<insert your name here>");
MODULE_DESCRIPTION("LKD book:ch8/oops_try: generates a kernel Oops! a kernel bug");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

/*static int num_burst_prints = 7;
module_param(num_burst_prints, int, 0644);
MODULE_PARM_DESC(num_burst_prints,
		 "Number of printk's to generate in a burst (defaults to 7).");
*/
static int __init try_oops_init(void)
{
	unsigned int randptr = 0x0;

	get_random_bytes(&randptr, sizeof(unsigned int));
	randptr %= PAGE_SIZE;
	pr_info("randptr = 0x%x\n", randptr);

	pr_info("Lets Oops!\nrandom kva lookup value is 0x%lx\n", *((unsigned int *)randptr));

	return 0;		/* success */
}

static void __exit try_oops_exit(void)
{
	pr_info("Goodbye, from Oops try\n");
}

module_init(try_oops_init);
module_exit(try_oops_exit);

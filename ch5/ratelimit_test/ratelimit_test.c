/*
 * ch5/ratelimit_test/ratelimit_test.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Debugging"
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Debugging
 *
 * From: Ch 5: Debug via Instrumentation - printk and friends
 ****************************************************************
 * Brief Description:
 * Quick test to see the behavior of the kernel printk when rate-limiting
 * is used.
 * Useful to limit / throttle down the same printk's being issued in bursts...
 *
 * For details, please refer the book, Ch 5.
 */
#define pr_fmt(fmt) "%s:%s():%d: " fmt, KBUILD_MODNAME, __func__, __LINE__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>

MODULE_AUTHOR("<insert your name here>");
MODULE_DESCRIPTION("LKD book:ch5/ratelimit_test: print a burst to test rate-limiting");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

static int num_burst_prints = 7;
module_param(num_burst_prints, int, 0644);
MODULE_PARM_DESC(num_burst_prints,
		 "Number of printk's to generate in a burst (defaults to 7).");

static int __init ratelimit_test_init(void)
{
	int i;

	pr_info("num_burst_prints=%d. Attempting to emit %d printk's in a burst:\n",
		num_burst_prints, num_burst_prints);

	for (i = 0; i < num_burst_prints; i++) {
		pr_info_ratelimited("[%d] ratelimited printk @ KERN_INFO [%d]\n", i,
				    LOGLEVEL_INFO);
		mdelay(100);	/* the delay helps magnify the rate-limiting effect, triggering the kernel's
				   "'n' callbacks suppressed" message... */
	}

	return 0;		/* success */
}

static void __exit ratelimit_test_exit(void)
{
	pr_info_ratelimited("Goodbye, ratelimited printk @ log-level KERN_INFO [%d]\n",
			    LOGLEVEL_INFO);
}

module_init(ratelimit_test_init);
module_exit(ratelimit_test_exit);

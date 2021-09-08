/*
 * ch5/printk_loglevels/printk_loglevels.c
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
 * Quick test to see kernel printk's at all 8 log levels (and the pr_devel());
 * when run on a console, only those messages < current (console) log level
 * (seen as the first integer in the output of /proc/sys/kernel/printk) will
 * appear on the console device.
 *
 * For details, please refer the book, Ch 5.
 */
#define pr_fmt(fmt) "%s:%s():%d: " fmt, KBUILD_MODNAME, __func__, __LINE__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_AUTHOR("<insert your name here>");
MODULE_DESCRIPTION("LKD book:ch5/printk_loglevels: print at each kernel log level");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

static int __init printk_loglevels_init(void)
{
	pr_emerg("Hello, debug world @ log-level KERN_EMERG   [%d]\n", LOGLEVEL_EMERG);
	pr_alert("Hello, debug world @ log-level KERN_ALERT   [%d]\n", LOGLEVEL_ALERT);
	pr_crit("Hello, debug world @ log-level KERN_CRIT    [%d]\n", LOGLEVEL_CRIT);
	pr_err("Hello, debug world @ log-level KERN_ERR     [%d]\n", LOGLEVEL_ERR);
	pr_warn("Hello, debug world @ log-level KERN_WARNING [%d]\n", LOGLEVEL_WARNING);
	pr_notice("Hello, debug world @ log-level KERN_NOTICE  [%d]\n", LOGLEVEL_NOTICE);
	pr_info("Hello, debug world @ log-level KERN_INFO    [%d]\n", LOGLEVEL_INFO);
	pr_debug("Hello, debug world @ log-level KERN_DEBUG   [%d]\n", LOGLEVEL_DEBUG);
	pr_devel("Hello, debug world via the pr_devel() macro (eff @KERN_DEBUG) [%d]\n", LOGLEVEL_DEBUG);

	return 0;		/* success */
}

static void __exit printk_loglevels_exit(void)
{
	pr_info("Goodbye, debug world @ log-level KERN_INFO    [%d]\n", LOGLEVEL_INFO);
}

module_init(printk_loglevels_init);
module_exit(printk_loglevels_exit);

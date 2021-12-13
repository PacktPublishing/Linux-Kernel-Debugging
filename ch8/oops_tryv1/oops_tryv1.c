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

static int __init try_oops_init(void)
{
	int *val = 0x0;

	pr_info("Lets Oops!\nNow attempting to write something to the NULL address 0x%p\n", NULL);
	*(int *)val = 'x';

	return 0;		/* success */
}

static void __exit try_oops_exit(void)
{
	pr_info("Goodbye, from Oops try\n");
}

module_init(try_oops_init);
module_exit(try_oops_exit);

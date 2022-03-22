/*
 * ch10/letspanic/letspanic.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Debugging"
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Debugging
 *
 * From: Ch 10: Kernel panic, hangcheck and watchdogs
 ****************************************************************
 * Brief Description:
 * "To conquer the beast one must first understand it. In that spirit, let' panic!"
 *
 * For details, please refer the book, Ch 10.
 */
#define pr_fmt(fmt) "%s:%s():%d: " fmt, KBUILD_MODNAME, __func__, __LINE__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_AUTHOR("<insert your name here>");
MODULE_DESCRIPTION("LKD book:ch10/letspanic: deliberately cause a kernel panic");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

static int myglobalstate = 0xeee;
static int __init letspanic_init(void)
{
	pr_warn("Hello, panic world (yay!)\n");
	panic("whoa, a kernel panic! myglobalstate = 0x%x", myglobalstate);

	return 0;		/* success */
}
module_init(letspanic_init);

// a quick trick- don't specify a cleanup function, as we'll never get there
#if 0
static void __exit letspanic_exit(void)
{
	pr_info("Goodbye, panic world\n");
}
module_exit(letspanic_exit);
#endif

/*
 * ch11/kgdb_try/kgdb_try.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Debugging"
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Debugging
 *
 * From: Ch 11: Using Kernel GDB (KGDB)
 ****************************************************************
 * Brief Description:
 *
 * For details, please refer the book, Ch 11.
 */
#define pr_fmt(fmt) "%s:%s():%d: " fmt, KBUILD_MODNAME, __func__, __LINE__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include "../../convenient.h"

MODULE_AUTHOR("<insert your name here>");
MODULE_DESCRIPTION("LKD book:ch11/kgdb_try: support for a simple demo using KGDB on a module");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

#define NUM  32
u8 gdata[NUM];

static void do_the_work(struct work_struct *);
static DECLARE_DELAYED_WORK(my_work, do_the_work);

/*
 * Our delayed workqueue callback function
 */
static void do_the_work(struct work_struct *work)
{
	u8 buf[10];
	int i;

	pr_info("In our workq function\n");
	for (i=0; i <=10; i++)
		buf[i] = (u8)i;
	print_hex_dump_bytes("", DUMP_PREFIX_OFFSET, buf, 10);

	/* Interesting! This bug (below) is caught at compile time on x86_64! Output:
	 * ...
	 * In function 'memset',
     * inlined from 'do_the_work' at <...>/ch11/kgdb_try/kgdb_try.c:46:2:
     * ./include/linux/string.h:381:3: error: call to '__write_overflow' declared with attribute error: detected write beyond size of object passed as 1st parameter
     * 381 |   __write_overflow();
	 * So, we leave it commented out...
	 */
	//memset(gdata, 0xff, NUM+PAGE_SIZE/2);
	pr_info("done\n");
}

static int __init kgdb_try_init(void)
{
	pr_info("Generating Oops via kernel bug in a delayed workqueue function\n");
	INIT_DELAYED_WORK(&my_work, do_the_work);
	schedule_delayed_work(&my_work, msecs_to_jiffies(2500));

	return 0;		/* success */
}

static void __exit kgdb_try_exit(void)
{
	cancel_delayed_work_sync(&my_work);
	pr_info("Goodbye\n");
}

module_init(kgdb_try_init);
module_exit(kgdb_try_exit);

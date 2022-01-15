/*
 * ch8/kcsan_datarace/kcsan_datarace.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Debugging"
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Debugging
 *
 * From: Ch 8: Lock Debugging
 ****************************************************************
 * Brief Description:
 *
 * For details, please refer the book, Ch 8.
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
MODULE_DESCRIPTION("LKD book:ch8/kcsan_datarace: deliberately generates a data race; KCSAN should catch it!");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

static bool race_2plain_w;
module_param(race_2plain_w, bool, 0644);
MODULE_PARM_DESC(race_2plain_w, "Trigger a data race due to plain writes on same address");

static struct st_ctx {
	struct work_struct work1, work2;
	u64 x, y, z, data;
} *gctx; /* careful, pointers have no memory! */

/*
 * Our workqueue callback function #1
 */
static void do_the_work1(struct work_struct *work1)
{
	int i;
	u64 bogus = 32000;

	PRINT_CTX();
	if (race_2plain_w) {
		pr_info("data race: 2 plain writes:\n");
		for (i=0; i<10000; i++) {
			gctx->data = bogus + i; /* unprotected plain write on global */
			mdelay(1);
		}
	}
}

/*
 * Our workqueue callback function #2
 */
static void do_the_work2(struct work_struct *work2)
{
	int i;

	PRINT_CTX();
	if (race_2plain_w) {
		pr_info("data race: 2 plain writes:\n");
		for (i=0; i<10000; i++) {
			gctx->data = (u64)gctx->y + i; /* unprotected plain write on global */
			mdelay(1);
		}
	}
}

static int setup_work(void)
{
	pr_info("global data item address: %px\n", &gctx->data);

	/* Initialize our workqueue #1 */
	INIT_WORK(&gctx->work1, do_the_work1);
	schedule_work(&gctx->work1);

	/* Initialize our workqueue #2 */
	INIT_WORK(&gctx->work2, do_the_work2);
	schedule_work(&gctx->work2);

	return 0;
}

static int __init kcsan_datarace_init(void)
{
	if (!race_2plain_w) {
		pr_info("nothing to do\n");
		return -EINVAL;
	}

	gctx = kzalloc(sizeof(struct st_ctx), GFP_KERNEL);
	if (!gctx)
		return -ENOMEM;
	
	gctx->data = 1;
	pr_info("Setting up a deliberate data race via our workqueue functions:\n");
	if (race_2plain_w == 1)
		pr_info("2 plain writes\n");

	setup_work();
	return 0;		/* success */
}

static void __exit kcsan_datarace_exit(void)
{
	kfree(gctx);
	pr_info("Goodbye\n");
}

module_init(kcsan_datarace_init);
module_exit(kcsan_datarace_exit);

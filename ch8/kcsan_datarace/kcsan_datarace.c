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
 * Here, we deliberately generate a data race by issuing concurrent plain
 * writes on the same address; KCSAN should catch it! So, of course, we assume
 * you're running this on a KCSAN-enabled debug kernel.
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

static int iter1 = 10000;
module_param(iter1, int, 0644);
MODULE_PARM_DESC(iter1, "# of times to loop in workfunc 1");

static int iter2 = 10000;
module_param(iter2, int, 0644);
MODULE_PARM_DESC(iter2, "# of times to loop in workfunc 2");

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
		for (i=0; i<iter1; i++)
			gctx->data = bogus + i; /* unprotected plain write on global */
	}
}

/*
 * Our workqueue callback function #2
 */
static void do_the_work2(struct work_struct *work2)
{
	int i;
	u64 bogus = 98000;

	PRINT_CTX();
	if (race_2plain_w) {
		pr_info("data race: 2 plain writes:\n");
		for (i=0; i<iter2; i++)
			gctx->data = bogus - i; /* unprotected plain write on global */
	}
}

static int setup_work(void)
{
	pr_info("global data item address: 0x%px\n", &gctx->data);

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
		pr_info("nothing to do (you're expected to set the module param race_2plain_w to True!)\n");
		return -EINVAL;
	}

	gctx = kzalloc(sizeof(struct st_ctx), GFP_KERNEL);
	if (!gctx)
		return -ENOMEM;
	
	gctx->data = 1;
	pr_info("Setting up a deliberate data race via our workqueue functions:\n");
	if (race_2plain_w == 1)
		pr_info("2 plain writes; #loops in workfunc1:%d workfunc2:%d\n", iter1, iter2);

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

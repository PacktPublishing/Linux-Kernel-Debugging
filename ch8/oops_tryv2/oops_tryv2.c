/*
 * ch8/oops_tryv2/oops_tryv2.c
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
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include "../../convenient.h"

MODULE_AUTHOR("<insert your name here>");
MODULE_DESCRIPTION("LKD book:ch7/oops_tryv2: generates a kernel Oops! a kernel bug in different ways");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

static unsigned long mp_randaddr;
module_param(mp_randaddr, ulong, 0644);
MODULE_PARM_DESC(mp_randaddr,
"Random non-zero kernel virtual address; deliberately invalid, to cause an Oops!");

static bool bug_in_workq;
module_param(bug_in_workq, bool, 0644);
MODULE_PARM_DESC(bug_in_workq, "Trigger an Oops-generating bug in our workqueue function");

static unsigned long bad_kva;
static u64 t1, t2;
static struct st_ctx {
	int x, y, z;
	struct work_struct work;
	u8 data;
} *gctx, *oopsie;

/*
 * Our workqueue callback function
 */
static void do_the_work(struct work_struct *work)
{
	struct st_ctx *priv = container_of(work, struct st_ctx, work);

	pr_info("In our workq function: data=%d\n", priv->data);
	PRINT_CTX();
	t2 = ktime_get_real_ns();
	SHOW_DELTA(t2, t1);
	if (!!bug_in_workq) {
		pr_info("Generating Oops by attempting to write to an invalid kernel memory pointer\n");
		oopsie->data = 'x';
	}
	kfree(gctx);
}

static int setup_work(void)
{
	gctx = kzalloc(sizeof(struct st_ctx), GFP_KERNEL);
	if (!gctx)
		return -ENOMEM;
	gctx->data = 'C';

	/* Initialize our workqueue */
	INIT_WORK(&gctx->work, do_the_work);
	// Do it!
	schedule_work(&gctx->work);

	return 0;
}

static int __init try_oops_init(void)
{
	unsigned int page0_randptr = 0x0;

	if (!!bug_in_workq) {
		pr_info("Generating Oops via kernel bug in workqueue function\n");
		t1 = ktime_get_real_ns();
		setup_work();
	}
	else if (mp_randaddr) {
		pr_info("Generating Oops by attempting to write to the invalid kernel address passed\n");
		bad_kva = mp_randaddr;
	} else {
		pr_info("Generating Oops by attempting to write to a random invalid kernel address in NULL trap page\n");
		get_random_bytes(&page0_randptr, sizeof(unsigned int));
		page0_randptr %= PAGE_SIZE;
		bad_kva = page0_randptr;
	}
	pr_info("bad_kva = 0x%lx; now writing to it...\n", bad_kva);
	*(unsigned long *)bad_kva = 0xdead;

	return 0;		/* success */
}

static void __exit try_oops_exit(void)
{
	pr_info("Goodbye, from Oops try v2\n");
}

module_init(try_oops_init);
module_exit(try_oops_exit);

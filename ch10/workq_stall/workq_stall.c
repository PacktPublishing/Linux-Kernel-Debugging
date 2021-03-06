/*
 * ch10/workq_stall/workq_stall.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Debugging"
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Debugging
 *
 * From: Ch 10 : Kernel panic, lockups and hangs
 ****************************************************************
 * Brief Description:
 * This code is originally from my earlier LKP-2 book, here:
 * https://github.com/PacktPublishing/Linux-Kernel-Programming-Part-2/tree/main/ch5/workq_simple
 * To simulate a workqueue stall, we insert a couple of lines of CPU-intensive
 * code in our kernel-default workqueue work function.
 * The kernel's workqueue stall etection code detects this - via the WQ watchdog
 * - and calls BUG() !
 *
 * Original comment:
 * A demo of a simple workqueue in action. We use the default kernel-global
 * workqueue; we enqueue a work item onto it (via INIT_WORK()) and 'schedule'
 * it to execute via the workqueue kthreads (using the schedule_work() API).
 * We cleanup (in the exit method) via the cancel_work_sync() API.
 * This module is built upon our earlier ch5/timer_simple LKM. Here, we also
 * demo making use of the container_of() macro to be able to access data from
 * within our work queue callback function - a pretty typical thing. Also, we
 * setup the timer to keep expiring until our 'data' variable hits zero; in
 * addition this time, we use this - the kernel timeout - as an opportunity to
 * 'schedule' our work queue function to run...
 *
 * For details, please refer the book, Ch 10.
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include "../../convenient.h"

#define INITIAL_VALUE	3

MODULE_AUTHOR("[insert name]");
MODULE_DESCRIPTION("a LKM to demo a simple workqueue stall");
MODULE_LICENSE("Dual MIT/GPL");	// or whatever
MODULE_VERSION("0.1");

static struct st_ctx {
	struct work_struct work;
	struct timer_list tmr;
	int data;
} ctx;
static unsigned long exp_ms = 420;
static u64 t1, t2;

/*
 * ding() - our timer's callback function!
 */
static void ding(struct timer_list *timer)
{
	struct st_ctx *priv = from_timer(priv, timer, tmr);

	pr_info("timed out... data=%d\n", priv->data);
	priv->data--;
	PRINT_CTX();

	/* until countdown done, fire it again! */
	if (priv->data)
		mod_timer(&priv->tmr, jiffies + msecs_to_jiffies(exp_ms));

	/* Now 'schedule' our workqueue function to run */
	if (!schedule_work(&priv->work)) {
		pr_notice("not added work item as it's already on the kernel-global workqueue!\n");
		return;
	}
	t1 = ktime_get_real_ns();
}

/*
 * work_func() - our workqueue callback function!
 */
static void work_func(struct work_struct *work)
{
	struct st_ctx *priv = container_of(work, struct st_ctx, work);
	u64 i = 0;

	t2 = ktime_get_real_ns();
	pr_info("In our workq function: data=%d\n", priv->data);
	PRINT_CTX();
	SHOW_DELTA(t2, t1);

	/* Deliberately spin for a loooong while... causing the kernel softlockup
	 * detector to swing into action!
	 */
	pr_info("Deliberately locking up the cpu now!\n");
	while (1)
		i += 3;
}

static int __init workq_simple_init(void)
{
	u64 i = 0;

	pr_info("Deliberately locking up the cpu now!\n");
	while (1)
		i ++;
	

	ctx.data = INITIAL_VALUE;

	/* Initialize our workqueue */
	INIT_WORK(&ctx.work, work_func);

	/* Initialize our kernel timer */
	ctx.tmr.expires = jiffies + msecs_to_jiffies(exp_ms);
	ctx.tmr.flags = 0;
	timer_setup(&ctx.tmr, ding, 0);

	pr_info("Work queue initialized, timer set to expire in %ld ms\n", exp_ms);
	add_timer(&ctx.tmr); /* Arm it; lets get going! */

	return 0;		/* success */
}

static void __exit workq_simple_exit(void)
{
	// Wait for any pending work (queue) to finish
	if (cancel_work_sync(&ctx.work))
		pr_info("yes, there was indeed some pending work; now done...\n");
	/* flush_scheduled_work(); Alternative, but harder to use correctly, as well
	 * as overkill; cancel_work_[delayed_]sync() is simpler.. */

	// Wait for possible timeouts to complete... and then delete the timer
	del_timer_sync(&ctx.tmr);
	pr_info("removed\n");
}

module_init(workq_simple_init);
module_exit(workq_simple_exit);

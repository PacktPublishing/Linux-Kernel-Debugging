/*
 * ch10/kthread_stuck/kthread_stuck.c
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
 * Added buggy code to deliberately spin on the CPU, in order to kick the
 * kernel watchdog into action and detect the softlockup caused!
 *
 * Original comment:
 * A simple LKM to demo delays and sleeps in the kernel.
 *
 * For details, please refer the book, Ch 10.
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/task.h>	// {get,put}_task_struct()
#include <linux/sched/signal.h> // signal_pending()
#include <linux/signal.h>       // allow_signal()
#include <linux/kthread.h>
#include <asm/atomic.h>
#include <linux/spinlock.h>
#include "../../convenient.h"

#define KTHREAD_NAME	"kt_stuck"

MODULE_AUTHOR("[insert name]");
MODULE_DESCRIPTION("a simple LKM to demo the kernel softlockup detector!");
MODULE_LICENSE("Dual MIT/GPL");	// or whatever
MODULE_VERSION("0.1");

static struct task_struct *gkthrd_ts;
static spinlock_t spinlock;

/* Our simple kernel thread. */
static int simple_kthread(void *arg)
{
	u64 i = 0;

	PRINT_CTX();
	if (!current->mm)
		pr_info("mm field NULL, we are a kernel thread!\n");

	/*
	 * By default all signals are masked for the kthread; allow a couple
	 * so that we can 'manually' kill it
	 */
	allow_signal(SIGINT);
	allow_signal(SIGQUIT);

	while(!kthread_should_stop()) {
		//------------------------------------
		pr_info("DELIBERATELY spinning on CPU core now...\n");
		spin_lock(&spinlock);
		while (i < 10000000000) {
			i ++;
			if (!(i%10000000)) pr_cont("%llu ", i);
		}
		spin_unlock(&spinlock);
		//------------------------------------

		pr_info("FYI, I, kernel thread PID %d, am going to sleep now...\n",
		    current->pid);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();	// yield the processor, go to sleep...
		/* Aaaaaand we're back! Here, it's typically due to either the
		 * SIGINT or SIGQUIT signal hitting us, or due to the rmmod (or shutdown)
		 */
		if (signal_pending(current))
			break;
	}

	// We've been (rudely) interrupted by a signal...
	set_current_state(TASK_RUNNING);
	pr_info("FYI, I, kernel thread PID %d, have been rudely awoken; I shall"
			" now exit... Good day Sir!\n", current->pid);

	return 0;
}

static int kthread_simple_init(void)
{
	int ret=0;

	spin_lock_init(&spinlock);
	pr_info("Lets now create a kernel thread...\n");

	/*
	 * kthread_run(threadfn, data, namefmt, ...)
	 * - it's just a thin wrapper over the kthread_create() API
	 * The 2nd arg is any (void * arg) to pass to the just-born kthread,
	 * and the return value is the task struct pointer on success
	 */
	gkthrd_ts = kthread_run(simple_kthread, NULL, "lkd/%s", KTHREAD_NAME);
	if (IS_ERR(gkthrd_ts)) {
		ret = PTR_ERR(gkthrd_ts); // it's usually -ENOMEM
		pr_err("kthread creation failed (%d)\n", ret);
		return ret;
	}
	get_task_struct(gkthrd_ts); /* increment the kthread task structure's
				      * reference count, marking it as being
				      * in use
				      */
	pr_info("Initialized, kernel thread task ptr is 0x%pK (actual=0x%px)\n"
	"See the new kernel thread 'lkd/%s' with ps (and kill it with SIGINT or SIGQUIT)\n",
		gkthrd_ts, gkthrd_ts, KTHREAD_NAME);

	return 0;		// success
}

static void kthread_simple_exit(void)
{
	kthread_stop(gkthrd_ts);
			/* waits for our kthread to terminate; it also
			 * internally invokes the put_task_struct() to
			 * decrement task's reference count
			 */
	pr_info("kthread stopped, and LKM removed.\n");
}

module_init(kthread_simple_init);
module_exit(kthread_simple_exit);

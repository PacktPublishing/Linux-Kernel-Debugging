/*
 * ch4/kprobes/1_kprobe/1_kprobe.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Debugging"
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Debugging
 *
 * From: Ch 4: Debug via Instrumentation - Kprobes
 ****************************************************************
 * Brief Description:
 * Traditional and manual approach, simplest case: attaching a kprobe,
 * hard-coding it (to the open system call).
 *
 * For details, please refer the book, Ch 4.
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/uaccess.h>
#include "../../../convenient.h"

MODULE_AUTHOR("<insert your name here>");
MODULE_DESCRIPTION("LKD book:ch4/kprobes/1_kprobe: simple Kprobes 1st demo module");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

static spinlock_t lock;
static struct kprobe kpb;
static u64 tm_start, tm_end;

/*
 * This probe runs just prior to the function "do_sys_open()" is invoked.
 */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	PRINT_CTX();	// uses pr_debug()

	spin_lock(&lock);
	tm_start = ktime_get_real_ns();
	spin_unlock(&lock);

	return 0;
}

/*
 * This probe runs immediately after the function "do_sys_open()" completes.
 */
static void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
	spin_lock(&lock);
	tm_end = ktime_get_real_ns();
	PRINT_CTX();	// uses pr_debug()
	SHOW_DELTA(tm_end, tm_start);
	spin_unlock(&lock);
	pr_debug("\n"); // silly- just to see the output clearly via dmesg/journalctl
}

/*
 * fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	pr_info("fault_handler: p->addr = 0x%p, trap #%dn", p->addr, trapnr);
	/* Return 0 because we don't handle the fault. */
	return 0;
}
NOKPROBE_SYMBOL(handler_fault);

static int __init kprobe_lkm_init(void)
{
	/* Register the kprobe handler */
	kpb.pre_handler = handler_pre;
	kpb.post_handler = handler_post;
	kpb.fault_handler = handler_fault;
	kpb.symbol_name = "do_sys_open";
	if (register_kprobe(&kpb)) {
		pr_alert("register_kprobe on do_sys_open() failed!\n");
		return -EINVAL;
	}
	pr_info("registering kernel probe @ 'do_sys_open()'\n");
	spin_lock_init(&lock);

	return 0;		/* success */
}

static void __exit kprobe_lkm_exit(void)
{
	unregister_kprobe(&kpb);
	pr_info("bye, unregistering kernel probe @ 'do_sys_open()'\n");
}

module_init(kprobe_lkm_init);
module_exit(kprobe_lkm_exit);

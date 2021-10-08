/*
 * ch5/kprobes/2_kprobe/2_kprobe.c
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
 * Traditional and manual approach: attaching a kprobe, slightly better,
 * soft-coding it via a module parameter (to the open system call);
 * via a module parameter.
 *
 * For details, please refer the book, Ch 5.
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
MODULE_DESCRIPTION("LKD book:ch5/2_kprobes/2_kprobe: simple Kprobes demo module with modparam");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

//#undef SKIP_IF_NOT_VI
#define SKIP_IF_NOT_VI

static spinlock_t lock;
static struct kprobe kpb;
static u64 tm_start, tm_end;

#define MAX_FUNCNAME_LEN  64
static char kprobe_func[MAX_FUNCNAME_LEN];
module_param_string(kprobe_func, kprobe_func, sizeof(kprobe_func), 0);
MODULE_PARM_DESC(kprobe_func, "function name to attach a kprobe to");

static int verbose;
module_param(verbose, int, 0644);
MODULE_PARM_DESC(verbose, "Set to 1 to get verbose printk's (defaults to 0).");

/*
 * This probe runs just prior to the function "kprobe_func()" is invoked.
 * Here, we're assuming you've setup a kprobe into the do_sys_open():
 *  long do_sys_open(int dfd, const char __user *filename, int flags, umode_t mode)
 */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
#ifdef SKIP_IF_NOT_VI
	/* For the purpose of this demo, we only log information when the process
	 * context is 'vi'
	 */
	if (strncmp(current->comm, "vi", 2))
		return 0;
#endif

	PRINT_CTX();
	spin_lock(&lock);
	tm_start = ktime_get_real_ns();
	spin_unlock(&lock);

	return 0;
}

/*
 * This probe runs immediately after the function "kprobe_func()" completes.
 */
static void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
#ifdef SKIP_IF_NOT_VI
    if (strncmp(current->comm, "vi", 2))
        return;
#endif

	spin_lock(&lock);
	tm_end = ktime_get_real_ns();

	if (verbose)
		PRINT_CTX();

	SHOW_DELTA(tm_end, tm_start);
	spin_unlock(&lock);
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
	/* Verify that the function to kprobe has been passed as a parameter to
	 * this module
	 */
	if (kprobe_func[0] == '\0') {
		pr_warn("expect a valid kprobe_func=<func_name> module parameter");
		return -EINVAL;
	}
	/********* Possible SECURITY concern:
     * We just assume the pointer passed is valid and okay, and not in the
	 * kprobes 'blacklist'.
	 * Minimally, ensure that the passed function is NOT marked with any of:
	 * __kprobes or nokprobe_inline annotation nor marked via the NOKPROBE_SYMBOL
	 * macro
	 */
	/* Register the kprobe handler */
	kpb.pre_handler = handler_pre;
	kpb.post_handler = handler_post;
	kpb.fault_handler = handler_fault;
	kpb.symbol_name = kprobe_func;
	if (register_kprobe(&kpb)) {
		pr_alert("register_kprobe failed!\n\
Check: is function '%s' invalid, static, inline; or blacklisted: attribute-marked '__kprobes'\n\
or nokprobe_inline, or is marked with the NOKPROBE_SYMBOL macro?\n", kprobe_func);
		return -EINVAL;
	}
	pr_info("registering kernel probe @ '%s'\n", kprobe_func);
	spin_lock_init(&lock);

	return 0;		/* success */
}

static void __exit kprobe_lkm_exit(void)
{
	unregister_kprobe(&kpb);
	pr_info("bye, unregistering kernel probe @ '%s'\n", kprobe_func);
}

module_init(kprobe_lkm_init);
module_exit(kprobe_lkm_exit);

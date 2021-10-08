/*
 * ch5/kprobes/4_kprobe_helper/helper_kp.c
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
 * Our kprobes demo #4:
 * Traditional, semi-automated manual approach: a helper script generates a
 * template for both the kernel module C code and the Makefile, enabling
 * attaching a kprobe to a given function via module parameter.
 *
 * This 'C' source will act as a template:
 * the helper script kp_load.sh will:
 *  copy it into a tmp/ folder (with name-timestamp.c format), a 
 *  Makefile built for it, it will be built (as a .ko) and, finally, 
 *  will be passed the name of a kernel or kernel module's function and 
 *  verbosity flag (during insmod).
 *
 * The job of this "helper" module is to setup the kprobe given the address.
 * The function must not be marked 'static' or 'inline' in the kernel / LKM.
 *
 * For details, please refer the book, Ch 5.
 * License: MIT
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include "../../../../convenient.h"

#define MODULE_VER 		"0.1"

static char *funcname;
/* module_param (var, type, sysfs_entry_permissions); 
 *  0 in last => no sysfs entry 
 */
module_param(funcname, charp, 0);
MODULE_PARM_DESC(funcname,
"Function name of the target (LKM's) function to attach probe to.");

static int verbose;
module_param(verbose, int, 0644);
MODULE_PARM_DESC(verbose, "Set to 1 to get verbose printk's (defaults to 0).");

static struct kprobe kpb;
static u64 tm_start = 0, tm_end = 0;
static int running_avg=0;
static spinlock_t lock;

/*
 * This probe runs just prior to the function "funcname()" is invoked.
 */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	spin_lock(&lock);
	tm_start = ktime_get_real_ns();
	spin_unlock(&lock);

	if (verbose) {
		pr_debug_ratelimited("%s:%s():Pre '%s'.\n", KBUILD_MODNAME, __func__, funcname);
		PRINT_CTX();
		//dump_stack();
	}
	return 0;
}

/*
 * This probe runs immediately after the function "funcname()" completes.
 */
static void handler_post(struct kprobe *p, struct pt_regs *regs,
		unsigned long flags)
{
	spin_lock(&lock);
	tm_end = ktime_get_real_ns();

	if (verbose) {
		pr_debug_ratelimited("%s:%s():%s:%d. Post '%s'.\n",
			KBUILD_MODNAME, __func__, current->comm, current->pid, funcname);
	}

	SHOW_DELTA(tm_end, tm_start);
	spin_unlock(&lock);
}

static int __init helper_kp_init_module(void)
{
	if (!funcname) {
		pr_info("%s:%s():Must pass funcname as a module parameter\n", KBUILD_MODNAME, __func__);
		return -EINVAL;
	}
	spin_lock_init(&lock);
	pr_info("%s:%s():kprobe'ing function %s, verbose mode %s\n",
		KBUILD_MODNAME, __func__, funcname, (verbose==1?"Y":"N"));

	/********* Possible SECURITY concern:
 	 * We just assume the pointer passed is valid and okay.
	 * Our kp_load.sh script has performed basic verification...
 	 */
	/* Register the kprobe handler */
	kpb.pre_handler = handler_pre;
	kpb.post_handler = handler_post;
	kpb.symbol_name = funcname;
	if (register_kprobe(&kpb)) {
		pr_alert("%s:%s():register_kprobe failed!\n"
		"Check: is function '%s' invalid, static, inline or attribute-marked '__kprobes' ?\n", 
			KBUILD_MODNAME, __func__, funcname);
		return -EINVAL;
	}
	pr_info("%s:%s():registered kprobe for function %s\n", KBUILD_MODNAME, __func__, funcname);
	return 0;	/* success */
}

static void helper_kp_cleanup_module(void)
{
	unregister_kprobe(&kpb);
	pr_info("%s:%s():unregistered kprobe @ function %s\n", KBUILD_MODNAME, __func__, funcname);
}

module_init(helper_kp_init_module);
module_exit(helper_kp_cleanup_module);

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("Helper Kprobe module; registers a kprobe to the passed function");
MODULE_LICENSE("Dual MIT/GPL");

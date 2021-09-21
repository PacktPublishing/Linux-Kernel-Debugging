/*
 * helper_kp.c v1.0 <date>
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
 * Kaiwan N Billimoria
 * <kaiwan -at- kaiwantech -dot- com>
 * License: MIT
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/kprobes.h>
#include "convenient.h"

#define MODULE_VER 		"0.1"
//#define MODULE_NAME    	"helper_kp"

static char * funcname;
/* module_param (var, type, sysfs_entry_permissions); 
 *  0 in last => no sysfs entry 
 */
module_param(funcname, charp, 0);
MODULE_PARM_DESC(funcname,
"Function name of the target (LKM's) function to attach probe to.");

static int verbose=0;
module_param(verbose, int, 0644);
MODULE_PARM_DESC(verbose, "Set to 1 to get verbose printk's (defaults to 0).");

static struct kprobe kpb;
static u64 tm_start = 0, tm_end = 0;
static int running_avg=0;
static spinlock_t lock;

/*
 * This probe runs just prior to "funcptr()" .
 */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	spin_lock(&lock);
	tm_start = ktime_get_real_ns();
	spin_unlock(&lock);

	if (verbose) {
		pr_debug( "Pre '%s'.\n", funcname);
		PRINT_CTX();
		//dump_stack();
	}
	return 0;
}

#define QUITE_LARGE_LATENCY_US	100  // time taken between the pre and post handler funcs
static void handler_post(struct kprobe *p, struct pt_regs *regs,
		unsigned long flags)
{
	spin_lock(&lock);
	tm_end = ktime_get_real_ns();

	if (verbose) {
		pr_debug( "%s:%d. Post '%s'.\n",
			current->comm, current->pid, funcname);
	}

/*
	timeval_subtract(&tv_diff, &tv_post, &tv_pre);
	running_avg = (running_avg + tv_diff.tv_usec)/2;
	if (unlikely(tv_diff.tv_sec > 0)) {
		MSG_SHORT( " delta: %ld s.", tv_diff.tv_sec);
	} else {
		MSG_SHORT( " delta: %2ld us, avg:%2d\n", tv_diff.tv_usec, running_avg);
	}
	if (unlikely(tv_diff.tv_usec > QUITE_LARGE_LATENCY_US))
		MSG_SHORT(" !!! %s:%d\n", current->comm, current->pid);
*/
	SHOW_DELTA(tm_end, tm_start);
	spin_unlock(&lock);
}

static int __init helper_kp_init_module(void)
{
	if (!funcname) {
		pr_info("Must pass funcname as a module parameter\n");
		return -EINVAL;
	}
	spin_lock_init(&lock);
	pr_info("trapping function %s, verbose mode %s\n", funcname, (verbose==1?"Y":"N"));

	/********* Possible SECURITY concern:
 	 * We just assume the pointer passed is valid and okay.
 	 */
	/* Register the kprobe handler */
	kpb.pre_handler = handler_pre;
	kpb.post_handler = handler_post;
	kpb.symbol_name = funcname;
	if (register_kprobe(&kpb)) {
		pr_alert("register_kprobe failed!\n"
		"Check: is function '%s' invalid, static, inline or attribute-marked '__kprobes' ?\n", 
			funcname);
		return -EINVAL;
	}
	pr_info("registered kprobe for function %s\n",  funcname);
	return 0;	/* success */
}

static void helper_kp_cleanup_module(void)
{
	unregister_kprobe(&kpb);
	pr_info("Unregistered kprobe @ function %s\n", funcname);
}

module_init(helper_kp_init_module);
module_exit(helper_kp_cleanup_module);

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("Helper Kprobe module; registers a kprobe to the passed function");
MODULE_LICENSE("Dual MIT/GPL");

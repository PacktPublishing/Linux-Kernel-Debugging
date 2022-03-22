/*
 * ch10/panic_notifier/panic_notifier_lkm.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Debugging"
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Debugging
 *
 * From: Ch 10: Kernel panic, hangcheck and watchdogs
 ****************************************************************
 * Brief Description:
 * This module registers a custom panic handler callback, via the atomic
 * type of notifier chain. It will be invoked upon kernel panic.
 *
 * For details, please refer the book, Ch 10.
 */
#define pr_fmt(fmt) "%s:%s():%d: " fmt, KBUILD_MODNAME, __func__, __LINE__
#include <linux/init.h>
#include <linux/module.h>
#include <linux/notifier.h>
//#include <asm-generic/bug.h>

MODULE_LICENSE("Dual MIT/GPL");

static u32 armsp;
static void arm_getreg(void)
{
#if defined CONFIG_ARM
	__asm__ __volatile__ (
		"mov r3, sp"
		: "=r" (armsp) /* output operands */
		:              /* input operands */
		: "r3");       /* clobbers */
#endif
}

static int mypanic(struct notifier_block *nb,
				  unsigned long code, void *_param)
{
	int loc=5;

	pr_emerg("Panic !ALARM! @ %s_%s_%d\n", __FILE__, __FUNCTION__, __LINE__);
	dump_stack();
#if defined CONFIG_ARM
	arm_getreg();
	pr_emerg("loc=0x%p, ARM stack ptr = 0x%x\n", &loc, armsp);
#endif
	return NOTIFY_OK;
}

static struct notifier_block mypanic_notifier_block = {
	.notifier_call = mypanic,
	.next = NULL,
	.priority = INT_MAX
};

static int __init panic_notifier_lkm_init(void)
{
    atomic_notifier_chain_register(&panic_notifier_list, &mypanic_notifier_block);
	pr_info("Registered panic notifier\n");

	return 0;  /* success */
}
static void __exit panic_notifier_lkm_exit(void)
{
    atomic_notifier_chain_unregister(&panic_notifier_list, &mypanic_notifier_block);
	pr_info("Unregistered panic notifier\n");
}

module_init(panic_notifier_lkm_init);
module_exit(panic_notifier_lkm_exit);

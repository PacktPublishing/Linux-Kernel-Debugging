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
 * This module is a simple PoC; it registers a custom panic handler callback,
 * by registering with the kernel's predefined panic notifier chain (an atomic
 * type of notifier chain). Thus, our custom panic handler will be invoked upon
 * kernel panic.
 *
 * For details, please refer the book, Ch 10.
 */
#define pr_fmt(fmt) "%s:%s():%d: " fmt, KBUILD_MODNAME, __func__, __LINE__
#include <linux/init.h>
#include <linux/module.h>
#include <linux/notifier.h>

/* The atomic_notifier_chain_[un]register() api's are GPL-exported! */
MODULE_LICENSE("Dual MIT/GPL");

/* Do what's required here for the product/project,
 * but keep it simple. Left essentially empty here..
 */
static void	dev_ring_alarm(void)
{
	pr_emerg("!!! ALARM !!!\n");
}

static int mypanic(struct notifier_block *nb, unsigned long code, void *data)
{
	pr_emerg("\n************ Panic : SOUNDING ALARM ************\n\
	code = %lu data:%s\n",
		code, data == NULL ? "<none>" : (char *)data);
	dev_ring_alarm();

	return NOTIFY_OK;
}

static struct notifier_block mypanic_nb = {
	.notifier_call = mypanic,
//	.priority = INT_MAX
};

static int __init panic_notifier_lkm_init(void)
{
	if (atomic_notifier_chain_register(&panic_notifier_list, &mypanic_nb);
	pr_info("Registered panic notifier\n");

	return 0;		/* success */
}

static void __exit panic_notifier_lkm_exit(void)
{
	atomic_notifier_chain_unregister(&panic_notifier_list, &mypanic_nb);
	pr_info("Unregistered panic notifier\n");
}

module_init(panic_notifier_lkm_init);
module_exit(panic_notifier_lkm_exit);

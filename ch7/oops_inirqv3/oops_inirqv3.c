/*
 * ch7/oops_inirqv3/oops_inirqv3.c
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
#include <linux/irq_work.h>
#include "../../convenient.h"

MODULE_AUTHOR("<insert your name here>");
MODULE_DESCRIPTION("LKD book:ch7/oops_inirqv3: generates a kernel Oops! while in interrupt context");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

static struct irq_work irqwork;

/*
 * This function runs in (hardirq) interrupt context
 */
void irq_work(struct irq_work *irqwk)
{
	int want_oops = 1;

	PRINT_CTX();
	if (!!want_oops) // okay, let's Oops in irq context!
		// a fatal hang can happen here!
		*(int *)0x100 = 'x';
}

static int __init try_oops_init(void)
{
	init_irq_work(&irqwork, irq_work);	
	irq_work_queue(&irqwork);

	return 0;		/* success */
}

static void __exit try_oops_exit(void)
{
	pr_info("Goodbye, from Oops in irq try v3\n");
}

module_init(try_oops_init);
module_exit(try_oops_exit);

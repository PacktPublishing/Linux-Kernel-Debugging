/*
 * ch7/kmembugs_test/kmembugs_test.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Debugging"
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Debugging
 *
 * From: Ch 7: Debugging kernel memory issues
 ****************************************************************
 * Brief Description:
 * This kernel module has buggy functions, each of which represents a simple
 * test case. They're deliberately selected to be the ones that are typically
 * NOT caught by KASAN!
 *
 * IMP:
 * By default, KASAN will turn off reporting after the very first error
 * encountered; we can change this behavior (and therefore test more easily)
 * by passing the kernel parameter kasan_multi_shot. Even easier, we can simply
 * first invoke the function kasan_save_enable_multi_shot() - which has the
 * same effect - and on unload restore it by invoking the
 * kasan_restore_multi_shot()! (note they require GPL licensing!).
 *
 * For details, please refer the book, Ch 7.
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("kmembugs_test: a couple of test cases for KASAN");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

#define NUM_ALLOC  64

static bool kasan_multishot;

/* The UMR - Uninitialized Memory Read - testcase */
static void umr(void)
{
	int x;
	/* V recent gcc does have the ability to detect this!
	 * To get warnings on this, you require to:
	 * a) use some optimization level besides 0 (-On, n != 0)
	 * b) pass the -Wuninitialized or -Wall compiler option.
	 * The gcc warning, when generated, typically shows up as:
	 *  warning: 'x' is used uninitialized in this function [-Wuninitialized]
	 *
	 * It carries a number of caveats though; pl see:
	 * https://gcc.gnu.org/wiki/Better_Uninitialized_Warnings
	 * Also see gcc(1) for the -Wmaybe-uninitialized option.
	 */

	if (x)
		pr_info("true case: x=%d\n", x);
	else
		pr_info("false case (x==0)\n");
}

/* The UAR - Use After Return - testcase */
static void *uar(void)
{
	char name[NUM_ALLOC];
#if 0				/* this too fails to be detected as a UAR ! */
	char *q = NULL;
	q = kzalloc(NUM_ALLOC, GFP_KERNEL);
	strncpy(q, "Linux kernel debug", 18);
	return q;
#endif
	memset(name, 0, NUM_ALLOC);
	strncpy(name, "Linux kernel debug", 18);

	return name;
	/*
	 * Here too, at the point of return, gcc emits a warning!
	 *  warning: function returns address of local variable [-Wreturn-local-addr]
	 * Good stuff.
	 */
}

/* A simple memory leakage testcase */
static int leak_simple(void)
{
	char *p = NULL;

	pr_info("simple memory leak testcase\n");
	p = kzalloc(1520, GFP_KERNEL);
	if (unlikely(!p))
		return -ENOMEM;
	print_hex_dump_bytes("p: ", DUMP_PREFIX_OFFSET, p, 32);

#if 0
	kfree(p);
#endif
	return 0;
}

static int __init kmembugs_test_init(void)
{
	int i, numtimes = 1;	// would you like to try a number of times? :)

	kasan_multishot = kasan_save_enable_multi_shot();
	for (i = 0; i < numtimes; i++) {
		char *res;

		// 1. Run the UMR - Uninitialized Memory Read - testcase
		umr();

		// 2. Run the UAR - Use After Return - testcase
		res = kmalloc(NUM_ALLOC, GFP_KERNEL);
		if (unlikely(!res))
			return -ENOMEM;
		res = uar();
		pr_info("res: %s\n", res == NULL ? "<whoops, it's NULL; UAR!>" : (char *)res);
		kfree(res);

		// 3. memleak
		leak_simple();
	}

	return 0;		/* success */
}

static void __exit kmembugs_test_exit(void)
{
	kasan_restore_multi_shot(kasan_multishot);
	pr_info("removed\n");
}

module_init(kmembugs_test_init);
module_exit(kmembugs_test_exit);

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
static int umr(void)
{
	volatile int x;
	/* Recent gcc does have the ability to detect this!
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

	return x;
}

/* The UAR - Use After Return - testcase */
static void *uar(void)
{
	volatile char name[NUM_ALLOC];
	volatile int i;

	for (i=0; i<NUM_ALLOC-1; i++)
		name[i] = 'x';
	name[i] = '\0';

	return name;
	/*
	 * Here too, at the point of return, gcc emits a warning!
	 *  warning: function returns address of local variable [-Wreturn-local-addr]
	 * Good stuff.
	 */
}

/* A simple memory leakage testcase 1 */
static void leak_simple1(void)
{
	char *p = NULL;

	p = kzalloc(1520, GFP_KERNEL);
	if (unlikely(!p))
		return;

	if (0) // test: ensure it isn't freed
		kfree(p);
}

/* A simple memory leakage testcase 2.
 * The caller's to free the memory...
 */
static void *leak_simple2(void)
{
	volatile char *q = NULL;
	volatile int i;
	volatile char heehee[] = "leaky!!";

#define NUM_ALLOC2	8
	q = kmalloc(NUM_ALLOC2, GFP_KERNEL);
	for (i=0; i<NUM_ALLOC2-1; i++)
			q[i] = heehee[i]; // 'x';
	q[i] = '\0';

	return (void *)q;
}

static int __init kmembugs_test_init(void)
{
	int i, numtimes = 1;	// would you like to try a number of times? :)

	kasan_multishot = kasan_save_enable_multi_shot();
	for (i = 0; i < numtimes; i++) {
		int umr_ret;
		char *res1 = NULL, *res2 = NULL;

		// 1. Run the UMR - Uninitialized Memory Read - testcase
		umr_ret = umr();
		pr_info("testcase 1: UMR (val=%d)\n", umr_ret);

		// 2. Run the UAR - Use After Return - testcase
		res1 = uar();
		pr_info("testcase 2: UAR: res1 = \"%s\"\n",
			res1 == NULL ? "<whoops, it's NULL; UAR!>" : (char *)res1);

		// 3. memory leak 1
		pr_info("testcase 3: simple memory leak testcase 1\n");
		leak_simple1();

		// 4. memory leak 2: caller's to free the memory!
		pr_info("testcase 4: simple memory leak testcase 2\n");
		res2 = (char *)leak_simple2();
		pr_info(" res2 = \"%s\"\n", res2 == NULL ? "<whoops, it's NULL; UAR!>" : (char *)res2);
		if (0) // test: ensure it isn't freed by the caller
			kfree(res2);
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

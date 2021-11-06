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
 * kmembugs_test.c: this source file:
 * This kernel module has buggy functions, each of which represents a simple
 * test case. Some of them are deliberately selected to be ones that are
 * typically NOT caught by KASAN!
 *
 * debugfs_kmembugs.c:
 * Source for the debugfs file - typically
 *  /sys/kernel/debug/test_kmembugs/lkd_dbgfs_run_testcase
 * Used to execute individual testcases by writing the testcase # (as a string)
 * to this pseudo-file.
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/debugfs.h>

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("kmembugs_test: a few additional test cases for KASAN/UBSAN");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

/*
static bool use_kasan_multishot;
module_param(use_kasan_multishot, bool, 0);
MODULE_PARM_DESC(use_kasan_multishot, "Set to 1 to run test cases for KASAN (default=0)");
*/
#ifdef CONFIG_KASAN
static bool kasan_multishot;
#endif

int debugfs_simple_intf_init(void);
extern struct dentry *gparent;

/*
 * All testcase functions are below:
 * they're deliberately _not_ marked with the static qualifier so that they're
 * accessible from the debugfs source file...
 */

/* The UMR - Uninitialized Memory Read - testcase */
int umr(void)
{
	volatile int x, y;
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
	pr_info("testcase 1: UMR (val=%d)\n", x);
	y = x;

	return x;
}

/* The UAR - Use After Return - testcase */
void *uar(void)
{
#define NUM_ALLOC  64
	volatile char name[NUM_ALLOC];
	volatile int i;

	pr_info("testcase 2: UAR:\n");
	for (i = 0; i < NUM_ALLOC - 1; i++)
		name[i] = 'x';
	name[i] = '\0';

	return (void *)name;
	/*
	 * Here too, at the point of return, gcc emits a warning!
	 *  warning: function returns address of local variable [-Wreturn-local-addr]
	 * Good stuff.
	 */
}

/* A simple memory leakage testcase 1 */
void leak_simple1(void)
{
	char *p = NULL;

	pr_info("testcase 3.1: simple memory leak testcase 1\n");
	p = kzalloc(1520, GFP_KERNEL);
	if (unlikely(!p))
		return;

	if (0)			// test: ensure it isn't freed
		kfree(p);
}

/* A simple memory leakage testcase 2.
 * The caller's to free the memory...
 */
void *leak_simple2(void)
{
	volatile char *q = NULL;
	volatile int i;
	volatile char heehee[] = "leaky!!";

	pr_info("testcase 3.2: simple memory leak testcase 2\n");
#define NUM_ALLOC2	8
	q = kmalloc(NUM_ALLOC2, GFP_KERNEL);
	for (i = 0; i < NUM_ALLOC2 - 1; i++)
		q[i] = heehee[i];	// 'x';
	q[i] = '\0';

	return (void *)q;
}

#define READ	0
#define WRITE	1

static char global_arr[10];

/*
 * OOB on static (compile-time) mem: OOB read/write (right) overflow
 * Covers both read/write overflow on both static global and local/stack memory
 */
int static_mem_oob_right(int mode)
{
	volatile char w, x, y, z;
	volatile char local_arr[20];

	if (mode == READ) {
		w = global_arr[ARRAY_SIZE(global_arr) - 2];	// valid and within bounds
		x = global_arr[ARRAY_SIZE(global_arr) + 2];	// invalid, not within bounds

		y = local_arr[ARRAY_SIZE(local_arr) - 5];	// valid and within bounds but random content!
		z = local_arr[ARRAY_SIZE(local_arr) + 5];	// invalid, not within bounds
		/* hey, there's also a lurking UMR defect here! local_arr[] has random content;
		 * KASAN/UBSAN don't seem to catch it; the compiler does! via a warning:
		 *  [...]warning: 'arr[20]' is used uninitialized in this function [-Wuninitialized]
		 *  142 |  x = arr[20]; // valid and within bounds
		 *      |      ~~~^~~~
		    ^^^^^^^^^^^^^^^^ ?? NOT getting gcc warning now!!
		 */
		//pr_info("global mem: w=0x%x x=0x%x; local mem: y=0x%x z=0x%x\n", w, x, y, z);
	}
	else if (mode == WRITE) {
		global_arr[ARRAY_SIZE(global_arr) - 2] = 'w';	// valid and within bounds
		global_arr[ARRAY_SIZE(global_arr) + 2] = 'x';	// invalid, not within bounds

		local_arr[ARRAY_SIZE(local_arr) - 5] = 'y';	// valid and within bounds but random content!
		local_arr[ARRAY_SIZE(local_arr) + 5] = 'z';	// invalid, not within bounds

	}
	return 0;
}

/********* TODO / RELOOK : KASAN isn't catching this !!! **************/
int static_mem_oob_left2(int mode)
{
	volatile char w, x, y, z;
	volatile char local_arr[20];
	volatile char *ptr = global_arr;

	memset((char *)ptr, 0x0, 10); //ARRAY_SIZE(global_arr));
	pr_info("global_arr=%px ptr=%px\n", global_arr, ptr);
	ptr = ptr - 16384;
	pr_info("ptr=%px\n", ptr);

	if (mode == READ) {
		w = *(volatile char *)ptr;
		pr_info("w=0x%x\n", w);
	}
	else if (mode == WRITE)
		*(volatile char *)ptr = 'x';
//	return 0;


	if (mode == READ) {
		w = global_arr[-2];	// invalid, not within bounds
		x = global_arr[2];	// valid, within bounds

		y = local_arr[-5];	// invalid, not within bounds and random!
		z = local_arr[5];	// valid, within bounds but random
		/* hey, there's also a lurking UMR defect here! local_arr[] has random content;
		 * KASAN/UBSAN don't seem to catch it; the compiler does! via a warning:
		 *  [...]warning: 'arr[20]' is used uninitialized in this function [-Wuninitialized]
		 *  142 |  x = arr[20]; // valid and within bounds
		 *      |      ~~~^~~~
		 */
	} else if (mode == WRITE) {
		global_arr[-2] = 'w'; // invalid, not within bounds
		global_arr[2] = 'x';  // valid, within bounds

		local_arr[-5] = 'y';  // invalid, not within bounds and random!
		local_arr[5] = 'z';	  // valid, within bounds but random
	}

	return 0;
}
/*
 * OOB on static (compile-time) mem: OOB read/write (left) underflow
 * Covers both read/write overflow on both static global and local/stack memory
 */
/********* TODO / RELOOK : KASAN isn't catching this !!! **************/
int static_mem_oob_left(int mode)
{
	volatile char w, x, y, z;
	volatile char local_arr[20];

	if (mode == READ) {
		w = global_arr[-2];	// invalid, not within bounds
		x = global_arr[2];	// valid, within bounds

		y = local_arr[-5];	// invalid, not within bounds and random!
		z = local_arr[5];	// valid, within bounds but random
		/* hey, there's also a lurking UMR defect here! local_arr[] has random content;
		 * KASAN/UBSAN don't seem to catch it; the compiler does! via a warning:
		 *  [...]warning: 'arr[20]' is used uninitialized in this function [-Wuninitialized]
		 *  142 |  x = arr[20]; // valid and within bounds
		 *      |      ~~~^~~~
		 */
	} else if (mode == WRITE) {
		global_arr[-2] = 'w'; // invalid, not within bounds
		global_arr[2] = 'x';  // valid, within bounds

		local_arr[-5] = 'y';  // invalid, not within bounds and random!
		local_arr[5] = 'z';	  // valid, within bounds but random
	}

	return 0;
}

/* OOB on dynamic (kmalloc-ed) mem: OOB read/write (right) overflow */
int dynamic_mem_oob_right(int mode)
{
	volatile char *kptr, ch = 0;
	size_t sz = 123;

	kptr = (char *)kmalloc(sz, GFP_KERNEL);
	if (!kptr)
		return -ENOMEM;

	if (mode == READ)
		ch = kptr[sz];
	else if (mode == WRITE)
		kptr[sz] = 'x';

	kfree((char *)kptr);
	return 0;
}

/* OOB on dynamic (kmalloc-ed) mem: OOB read/write (left) underflow */
int dynamic_mem_oob_left(int mode)
{
	volatile char *kptr, *ptr, ch = 'x';
	size_t sz = 123;

	kptr = (char *)kmalloc(sz, GFP_KERNEL);
	if (!kptr)
		return -ENOMEM;
	ptr = kptr - 1;

	if (mode == READ)
		ch = *(volatile char *)ptr;
	else if (mode == WRITE)
		*(volatile char *)ptr = ch;

	kfree((char *)kptr);
	return 0;
}

/* Use After Free - UAF - defect testcase */
int uaf(void)
{
	volatile char *kptr, *ptr;
	size_t sz = 123;

	kptr = (char *)kmalloc(sz, GFP_KERNEL);
	if (!kptr)
		return -ENOMEM;
	ptr = kptr + 3;

	*(volatile char *)ptr = 'x';
	kfree((char *)kptr);
	ptr = kptr + 8;
	*(volatile char *)ptr = 'y'; // the bug

	return 0;
}

/* Double free defect testcase */
int double_free(void)
{
	volatile char *kptr, *ptr;
	size_t sz = 123;

	kptr = (char *)kmalloc(sz, GFP_KERNEL);
	if (!kptr)
		return -ENOMEM;
	ptr = kptr + 3;

	*(volatile char *)ptr = 'x';
	kfree((char *)kptr);
	if (1)	// the bug
		kfree((char *)kptr);

	return 0;
}


static int __init kmembugs_test_init(void)
{
	int stat;

	pr_info("Testing via ");
		/*
		 * Realize that the kasan_save_enable_multi_shot() / kasan_restore_multi_shot()
		 * pair of functions work only on a kernel that has CONFIG_KASAN=y. Also,
		 * we're expecting Generic KASAN enabled.
		 */
#ifdef CONFIG_KASAN_GENERIC
		kasan_multishot = kasan_save_enable_multi_shot();
		pr_info("KASAN");
#else
		pr_warn("Attempting to test for KASAN on a non-KASAN-enabled kernel!\n");
//		return -EINVAL;
#endif
	if (IS_ENABLED(CONFIG_UBSAN))
		pr_info("|UBSAN\n");
	else
		pr_info("\n");

	stat = debugfs_simple_intf_init();
	if (stat < 0)
		return stat;

	return 0;		/* success */

#if 0
	for (i = 0; i < numtimes; i++) {
		int umr_ret;
		char *res1 = NULL, *res2 = NULL;

		// 1. Run the UMR - Uninitialized Memory Read - testcase
		umr_ret = umr();
		//pr_info("testcase 1: UMR (val=%d)\n", umr_ret);

		// 2. Run the UAR - Use After Return - testcase
		res1 = uar();
		pr_info("testcase 2: UAR: res1 = \"%s\"\n",
			res1 == NULL ? "<whoops, it's NULL; UAR!>" : (char *)res1);

		// 3. Run the UAF - Use After Free - testcase

		//---------- 4. OOB accesses on static memory (read/write under/overflow)
		pr_info
		    ("testcases set 4: simple OOB accesses on static memory (read/write under/overflow)\n");
		pr_info(" 4.1: static (compile-time) mem: OOB read (right) overflow\n");
//		static_mem_oob_right(READ);
		pr_info(" 4.2: static (compile-time) mem: OOB write (right) overflow\n");
		static_mem_oob_right(WRITE);
		pr_info(" 4.3: static (compile-time) mem: OOB read (left) underflow\n");
//		static_mem_oob_left(READ);
		pr_info(" 4.4: static (compile-time) mem: OOB write (left) underflow\n");
		static_mem_oob_left(WRITE);

		/*
		   oob_array_dynmem();

		   // 5. OOB static array access
		   pr_info("testcase 6: simple OOB memory access on static (compile-time) memory array\n");
		   oob_array_staticmem();
		 */

		// 6.1. memory leak 1
		pr_info("testcase 6.1: simple memory leak testcase 1\n");
		leak_simple1();

		// 6.2. memory leak 2: caller's to free the memory!
		pr_info("testcase 6.2: simple memory leak testcase 2\n");
		res2 = (char *)leak_simple2();
		pr_info(" res2 = \"%s\"\n",
			res2 == NULL ? "<whoops, it's NULL; UAR!>" : (char *)res2);
		if (0)		// test: ensure it isn't freed by the caller
			kfree(res2);
	}

	return 0;		/* success */
#endif
}

static void __exit kmembugs_test_exit(void)
{
#ifdef CONFIG_KASAN_GENERIC
	kasan_restore_multi_shot(kasan_multishot);
#endif
	debugfs_remove_recursive(gparent);
	pr_info("removed\n");
}

module_init(kmembugs_test_init);
module_exit(kmembugs_test_exit);

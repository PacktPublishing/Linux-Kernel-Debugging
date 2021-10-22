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
 *
 * IMP:
 *  By default, KASAN will turn off after the very first error encountered;
 *  can change this behavior (and therefore test more easily, instead of
 *  constantly rebooting) by passing the kernel parameter kasan_multi_shot
 *  on the kernel cmd line.
 *
 * For details, please refer the book, Ch 7.
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("membugs_kasan: a few KASAN test cases");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");

#define NUM  2048

#define FATAL(errmsg, ern) do {       \
	pr_warn("%s: Fatal Error! \"%s\"\n", \
		KBUILD_MODNAME, errmsg);          \
		return ern;                   \
} while (0)

static int testcase;
module_param(testcase, int, 0644);
MODULE_PARM_DESC(testcase, " Test case to run ::\n\
 1 : UMR : uninitialized memory read\n\
 2 : OOB - out-of-bounds : write overflow on compile-mem\n\
 3 : OOB - out-of-bounds : write overflow on dynamic-mem\n\
 4 : OOB - out-of-bounds : write underflow\n\
 5 : OOB - out-of-bounds : read overflow on compile-mem\n\
 6 : OOB - out-of-bounds : read overflow on dynamic-mem\n\
 7 : OOB - out-of-bounds : read underflow\n\
 8 : UAF - use-after-free\n\
 9 : UAR - use-after-return\n\
10 : double-free \n\
11 : memory leak : simple leak\n\
");

/*
 * Legend (CSV) ::
 *   test case, KASAN catches it?, slub_debug= catches it?, vanilla kernel catches it?

 *	" test case  1 : uninitialized var test case\n" , N,
	" test case  2 : out-of-bounds : write overflow [on compile-time memory]\n", N,
	" test case  3 : out-of-bounds : write overflow [on dynamic memory]\n", Y,
	" test case  4 : out-of-bounds : write underflow\n", Y,
	" test case  5 : out-of-bounds : read overflow [on compile-time memory]\n", Y,
	" test case  6 : out-of-bounds : read overflow [on dynamic memory]\n", Y,
	" test case  7 : out-of-bounds : read underflow\n", Y,
	" test case  8 : UAF (use-after-free) test case\n", Y,
	" test case  9 : UAR (use-after-return) test case\n", N,
	" test case 10 : double-free test case\n", Y,
	" test case 11 : memory leak test case: simple leak\n", N,
 */

/*-------------------------------------------------------------------*/
static int leak_simple(void)
{
	char *p = NULL;

	p = kzalloc(1520, GFP_KERNEL);
	if (!p)
		FATAL("testcase #11, kzalloc failed!", -ENOMEM);
	print_hex_dump_bytes("p: ", DUMP_PREFIX_OFFSET, p, 32);

#if 0
	kfree(p);
#endif
	return 0;
}

static int double_free(void)
{
	char *p = NULL, *q = NULL;
	char src[] = "abcd5678";

	p = kzalloc(320, GFP_KERNEL);
	if (!p)
		FATAL("testcase #10, kzalloc 1 failed!", -ENOMEM);

	strncpy(p, src, strlen(src));
	kfree(p);

	q = kzalloc(-1UL, GFP_KERNEL);	/* -1UL becomes 2^64 (or 2^32, depending);
					   thus it's impossible that the kzalloc succeeds */
	if (!q) {		/* WILL happen */
		kfree(p);	/* Bug: double free! */
		FATAL("testcase #10, kzalloc 2 failed!", -ENOMEM);
	}

	return 0;
}

static void *uar(void)
{
	char name[32];
#if 0				/* this too fails to be detected as a UAR ! */
	char *q = NULL;
	q = kzalloc(32, GFP_KERNEL);
	strncpy(q, "Linux kernel debug", 18);
	return q;
#endif
	memset(name, 0, 32);
	strncpy(name, "Linux kernel debug", 18);

	return name;
}

static int uaf(void)
{
	char *p = kzalloc(32, GFP_KERNEL);
	char src[] = "abcd5678";
	int qs = 1;

	if (!p)
		FATAL("testcase 8: kzalloc failed!", -ENOMEM);
	strncpy(p, src, strlen(src));
	kfree(p);

	if (qs) {
		strncpy(p, src, strlen(src));	/* UAF ! */
		//pr_info("p = %s\n", p);
	}
	return 0;
}

static void oob_read_underflow(char **p, int sz)
{
	pr_info
	    ("** Test case :: OOB read underflow  dynamic-mem [func %s()] **\n",
	     __func__);
	pr_debug("*p = %p\n", (void *)*p);
	//pr_debug("*p = 0x%lx\n", (void *)*p);
	pr_info("reading at kva %p+%+d : 0x%x\n", (void *)*p, sz, *(*p + sz));	// sz is passed as -ve
}

static void oob_read_overflow_dynmem(char **p, int sz)
{
	pr_info
	    ("** Test case :: OOB read overflow dynamic-mem [func %s()] **\n",
	     __func__);
	pr_debug("*p = %p\n", (void *)*p);
	pr_info("reading at kva %p+%+d : 0x%x\n", (void *)*p, sz, *(*p + sz));	// sz is passed as +ve
}

static void oob_read_overflow_compilemem(void)
{
	int i, arr[5], arr2[7];

	pr_info
	    ("** Test case :: OOB read overflow compile-mem [func %s()] **\n",
	     __func__);
	for (i = 0; i < 5; i++)
		arr[i] = i;

	for (i = 0; i <= 10; i++)
		pr_info("arr[%d] = %d\n", i, arr[i]);
}

static void oob_write_underflow(char **p, int sz)
{
	pr_info
	    ("** Test case :: OOB write underflow  dynamic-mem [func %s()] **\n",
	     __func__);
	pr_debug("*p = %p\n", (void *)*p);
	memset(*p + sz, 0xab, 32);	// parameter 'sz' is passed as -ve
}

static void oob_write_overflow_dynmem(char **p, int sz)
{
	pr_info
	    ("** Test case :: OOB write underflow dynamic-mem [func %s()] **\n",
	     __func__);
	pr_debug("*p = %p\n", (void *)*p);
	memset(*p, 0xab, NUM + sz);
}

/* test case : out-of-bounds : write overflow [on compile-time memory] */
static void oob_write_overflow_compilemem(void)
{
	int i, arr[5], arr2[7];
	static int s_arr[5], s_arr2[10];

	pr_info
	    ("** Test case :: OOB write overflow compile-mem [func %s()] **\n",
	     __func__);
	//for (i = 0; i <= 5; i++) {
	//for (i = 0; i <= 50; i++) {
	for (i = 0; i <= 5000; i++) {
		arr[i] = 100;	/* Bug: 'arr' overflows on i==5,
				   overwriting part of the 'arr2'
				   variable - a stack overflow! */
		s_arr[i] = 200;
	}
}

/* test case : UMR - uninitialized memory read - test case */
static void umr(void)
{
	int x; /* v recent gcc does have the ability to detect this!
	 warning: ‘x’ is used uninitialized in this function [-Wuninitialized]
    */

	if (x)
		pr_info("%s(): true case: x=%d\n", __func__, x);
	else
		pr_info("%s(): false case (x==0)\n", __func__);
}

/*-------------------------------------------------------------------*/

static void *init(void)
{
	char *kp = NULL;

	kp = kvmalloc(NUM, GFP_KERNEL);
	if (!kp) {
		pr_warn("kmalloc failed\n"); // pedantic..
		return ERR_PTR(-ENOMEM);
	}
	pr_debug("kp = 0x%px\n", kp);
		/* NOTE : Security warning via checkpatch:
WARNING: Using vsprintf specifier '%px' potentially exposes the kernel memory layout, if you don't really need the address please consider using '%p'."
		*/
#if 0
	pr_info(" kp = %lx; dumping first 32 bytes Before init...\n", kp);
	print_hex_dump_bytes("kp: ", DUMP_PREFIX_OFFSET, kp, 32);
#if 1
	memset(kp, 0xdd, NUM);
#else
	memset(kp, 0xdd, NUM + 4);
#endif
	pr_info(" dumping first 32 bytes After init...\n");
	print_hex_dump_bytes("kp: ", DUMP_PREFIX_ADDRESS, kp, 32);
#endif
	return kp;
}

static int __init kmembugs_test_init(void)
{
	char *kp = NULL, *res;

	pr_info("inserted: testcase = %d\n", testcase);
	if (testcase == 0) {
		pr_info
		    ("requires the module parameter 'testcase' to be passed. Aborting...\n");
		return -EINVAL;
	}
	kp = init();

	switch (testcase) {
	case 1:
		umr();
		break;
	case 2:
		oob_write_overflow_compilemem();
		break;
	case 3:
		oob_write_overflow_dynmem(&kp, 8);
		break;
	case 4:
		oob_write_underflow(&kp, -8);
		break;
	case 5:
		oob_read_overflow_compilemem();
		break;
	case 6:
		oob_read_overflow_dynmem(&kp, NUM + 8);
		break;
	case 7:
		oob_read_underflow(&kp, -NUM - 8);
		break;
	case 8:
		uaf();
		break;
	case 9:
		res = kmalloc(32, GFP_KERNEL);
		res = uar();
		pr_info(" res: %s\n", (char *)res);
		kfree(res);
		break;
	case 10:
		double_free();
		break;
	case 11:
		leak_simple();
		break;
	default:
		pr_info("Invalid testcase # %d passed, aborting...\n",
			testcase);
		break;
	}

	kfree(kp);
	return 0;		/* success */
}

static void __exit kmembugs_test_exit(void)
{
	pr_info("removed\n");
}

module_init(kmembugs_test_init);
module_exit(kmembugs_test_exit);

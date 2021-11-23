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
 * test case.
 *
 * debugfs_kmembugs.c:
 * Source for the debugfs infrastructure to run these test cases; it creates the
 * debugs file - typically
 *  /sys/kernel/debug/test_kmembugs/lkd_dbgfs_run_testcase
 * used to execute individual testcases by writing the testcase # (as a string)
 * to this pseudo-file.
 *
 * IMP:
 * By default, KASAN will turn off reporting after the very first error
 * encountered; we can change this behavior (and therefore test more easily)
 * by using the API pair kasan_save_enable_multi_shot() / kasan_restore_multi_shot()
 * we do just this within the init and cleanup of this module
 * (note that these APIs require GPL licensing!).
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

/* The UMR - Uninitialized Memory Read - testcase on kernel stack (local) memory */
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

/* The UMR - Uninitialized Memory Read - testcase on kernel slab (dynamic) memory */
int umr_slub(void)
{
	volatile char *q = NULL;

	pr_info("testcase 10: simple UMR on slab memory\n");
	q = kmalloc(32, GFP_KERNEL);
	if (unlikely(!q))
		return -ENOMEM;
	pr_info("q[3] is 0x%x\n", q[3]);	//*(q+3));
	print_hex_dump_bytes("q: ", DUMP_PREFIX_OFFSET, (void *)q, 32);
	kfree((char *)q);

	return 0;
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
		q[i] = heehee[i];
	q[i] = '\0';

	return (void *)q;
}

#define READ	0
#define WRITE	1

/* Observations on KASAN catching OOB accesses on global (static) memory:
 * a) requires compilation by clang 11 or later
 * b) the way the internal redzoning and padding seems to work, it appears that
 *    the first declared global (this depends on how the linker sets it up), may
 *    not have a left redzone, causing left OOBs to be missed... This is why we
 *    use three global arrays; we'll pass the middle one; hopefully, it's
 *    properly redzoned and OOB accesses caught.
 */
#define ARRSZ	10
char global_arr1[ARRSZ];
char global_arr2[ARRSZ];
char global_arr3[ARRSZ];

/*
 * OOB on static (compile-time) mem: OOB read/write (right) overflow.
 * Covers both read/write overflow on both static global and local/stack memory.
 * The parameter p is a pointer to one of the global memory arrays we have in
 * this module.
 * Note: With gcc 10, 11 or clang < 11, KASAN isn't catching static global
 * memory OOB on read/write underflow!
 */
int global_mem_oob_right(int mode, char *p)
{
	volatile char w, x, y, z;
	volatile char local_arr[20];
	char *volatile ptr = p + ARRSZ + 3; // OOB right

	if (mode == READ) {
		w = *(volatile char *)ptr;	// invalid, OOB right read
		ptr = p + 3;
		x = *(volatile char *)ptr;	// valid

		y = local_arr[ARRAY_SIZE(local_arr) - 5];	// valid and within bounds but random content!
		z = local_arr[ARRAY_SIZE(local_arr) + 5];	// invalid, OOB right read and random
	} else if (mode == WRITE) {
		*(volatile char *)ptr = 'x';	// invalid, OOB right write

		p[ARRSZ - 3] = 'w';	// valid and within bounds
		p[ARRSZ + 3] = 'x';	// invalid, OOB right write

		local_arr[ARRAY_SIZE(local_arr) - 5] = 'y';	// valid and within bounds
		local_arr[ARRAY_SIZE(local_arr) + 5] = 'z';	// invalid, OOB right write
	}
	return 0;
}

/*
 * OOB on static (compile-time) mem: OOB read/write (left) underflow.
 * Covers both read/write overflow on both static global and local/stack memory.
 * The parameter p is a pointer to one of the global memory arrays we have in
 * this module.
 * Note: With gcc 10, 11 or clang < 11, KASAN isn't catching static global
 * memory OOB on read/write underflow!
 */
int global_mem_oob_left(int mode, char *p)
{
	volatile char w, x, y, z;
	volatile char local_arr[20];
	char *volatile ptr = p - 3; // left OOB

	if (mode == READ) {
		/* Interesting: this OOB access isn't caught by UBSAN but is caught by KASAN! */
		w = *(volatile char *)ptr;	// invalid, OOB left read

		/* ... but these below OOB accesses are caught by UBSAN.
		 * We conclude that *only* the index-based accesses are caught by UBSAN.
		 * And, KASAN compiled with clang 11 or later, can catch the pointer-based OOB above!
		 */
		x = p[-3];	// invalid, OOB left read

		y = local_arr[-5];	// invalid, not within bounds and random!
		z = local_arr[5];	// valid, within bounds but random content
	} else if (mode == WRITE) {
		/* Interesting: this OOB access isn't caught by UBSAN but is caught by KASAN! */
		*(volatile char *)ptr = 'w';

		/* ... but these below OOB accesses are caught by UBSAN.
		 * We conclude that *only* the index-based accesses are caught by UBSAN.
		 * And, KASAN compiled with clang 11 or later, can catch the pointer-based OOB above!
		 */
		ptr[0] = 'w';  // invalid, OOB left write
		ptr[5] = 'x';  // valid, within bounds

		local_arr[-5] = 'y'; // invalid, not within bounds
		local_arr[5] = 'z';	 // valid, within bounds
	}
	return 0;
}

/* OOB on dynamic (kmalloc-ed) mem: OOB read/write (right) overflow */
int dynamic_mem_oob_right(int mode)
{
	volatile char *kptr, ch = 0;
	char *volatile ptr;
	size_t sz = 32;

	kptr = (char *)kmalloc(sz, GFP_KERNEL);
	if (unlikely(!kptr))
		return -ENOMEM;
	ptr = (char *)kptr + sz + 3; // right OOB

	if (mode == READ) {
		/* Interesting: this OOB access isn't caught by UBSAN but is caught by KASAN! */
		ch = *(volatile char *)ptr; // invalid, OOB right write
		/* ... but these below OOB accesses are caught by KASAN/UBSAN.
		 * We conclude that *only* the index-based accesses are caught by UBSAN.
		 */
		ch = kptr[sz + 3];	// invalid, OOB right read
	} else if (mode == WRITE) {
		/* Interesting: this OOB access isn't caught by UBSAN but is caught by KASAN! */
		*(volatile char *)ptr = 'x';
		/* ... but these below OOB accesses are caught by KASAN/UBSAN.
		 * We conclude that *only* the index-based accesses are caught by UBSAN.
		 */
		kptr[sz] = 'x';	// invalid, OOB right write
	}

	kfree((char *)kptr);
	return 0;
}

/* OOB on dynamic (kmalloc-ed) mem: OOB read/write (left) underflow */
int dynamic_mem_oob_left(int mode)
{
	volatile char *kptr, *ptr, ch = 'x';
	size_t sz = 32;

	kptr = (char *)kmalloc(sz, GFP_KERNEL);
	if (unlikely(!kptr))
		return -ENOMEM;
	ptr = kptr - 1; // OOB left

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
	size_t sz = 32;

	kptr = (char *)kmalloc(sz, GFP_KERNEL);
	if (unlikely(!kptr))
		return -ENOMEM;
	ptr = kptr + 3;

	*(volatile char *)ptr = 'x';
	kfree((char *)kptr);
	ptr = kptr + 8;
	*(volatile char *)ptr = 'y';	// the bug

	return 0;
}

/* Double free defect testcase */
int double_free(void)
{
	volatile char *kptr, *ptr;
	size_t sz = 32;

	kptr = (char *)kmalloc(sz, GFP_KERNEL);
	if (unlikely(!kptr))
		return -ENOMEM;
	ptr = kptr + 3;

	*(volatile char *)ptr = 'x';
	kfree((char *)kptr);
	if (1)			// the bug
		kfree((char *)kptr);

	return 0;
}

/*------------------ UBSAN Arithmetic UB testcases ----------------------------
 * All copied verbatim from the kernel src tree here:
 *  lib/test_ubsan.c
 */
void test_ubsan_add_overflow(void)
{
	volatile int val = INT_MAX;

	val += 2;
}

void test_ubsan_sub_overflow(void)
{
	volatile int val = INT_MIN;
	volatile int val2 = 2;

	val -= val2;
}

void test_ubsan_mul_overflow(void)
{
	volatile int val = INT_MAX / 2;

	val *= 3;
}

void test_ubsan_negate_overflow(void)
{
	volatile int val = INT_MIN;

	val = -val;
}

void test_ubsan_divrem_overflow(void)
{
	volatile int val = 16;
	volatile int val2 = 0;

	val /= val2;
}

void test_ubsan_shift_out_of_bounds(void)
{
	volatile int val = -1;
	int val2 = 10;

	val2 <<= val;
}

void test_ubsan_out_of_bounds(void)
{
	volatile int i = 4, j = 5;
	volatile int arr[4];

	arr[j] = i;
}

void test_ubsan_load_invalid_value(void)
{
	volatile char *dst, *src;
	bool val, val2, *ptr;
	char c = 4;

	dst = (char *)&val;
	src = &c;
	*dst = *src;

	ptr = &val2;
	val2 = val;
}

void test_ubsan_null_ptr_deref(void)
{
	volatile int *ptr = NULL;
	int val;

	val = *ptr;
}

void test_ubsan_misaligned_access(void)
{
	volatile char arr[5] __aligned(4) = { 1, 2, 3, 4, 5 };
	volatile int *ptr, val = 6;

	ptr = (int *)(arr + 1);
	*ptr = val;
}

void test_ubsan_object_size_mismatch(void)
{
	/* "((aligned(8)))" helps this not into be misaligned for ptr-access. */
	volatile int val __aligned(8) = 4;
	volatile long long *ptr, val2;

	ptr = (long long *)&val;
	val2 = *ptr;
}

/*---------------- end UBSAN testcases ---------------------------------------*/

/*
 * Testcase for the [__]copy_[to|from]_user[_inatomic]() routines for OOB accesses.
 * Copied verbatim from lib/test_kasan_module.c
 */
#include <uapi/asm-generic/mman-common.h>
#include <uapi/linux/mman.h>
#define KASAN_SHADOW_SCALE_SIZE  3
/* Due to our being out-of-tree, attmepting to use kernel macros won't always
 * work well; so we just hard-code this to the expected value...
 *  (KASAN_SHADOW_SCALE_SIZE = 1UL << KASAN_SHADOW_SCALE_SHIFT
 *   and KASAN_SHADOW_SCALE_SHIFT = 3)
 */
#define OOB_TAG_OFF (IS_ENABLED(CONFIG_KASAN_GENERIC) ? 0 : KASAN_SHADOW_SCALE_SIZE)
noinline void oob_copy_user_test(void)
{
	char *kmem;
	char __user *usermem;
	size_t size = 10;
	int unused;

	kmem = kmalloc(size, GFP_KERNEL);
	if (unlikely(!kmem))
		return;

	usermem = (char __user *)vm_mmap(NULL, 0, PAGE_SIZE,
					 PROT_READ | PROT_WRITE | PROT_EXEC,
					 MAP_ANONYMOUS | MAP_PRIVATE, 0);
	if (IS_ERR(usermem)) {
		pr_err("Failed to allocate user memory\n");
		kfree(kmem);
		return;
	}
#if 0
	/* Skipping these two as the compiler itself (quite cleverly) catches them!
	 * This is the gcc output: [...]
	 * In function 'check_copy_size',
	 inlined from 'copy_from_user' at ./include/linux/uaccess.h:191:6,
	 inlined from 'copy_user_test' at /home/letsdebug/Linux-Kernel-Debugging/ch7/kmembugs_test/kmembugs_test.c:482:14:
	 ./include/linux/thread_info.h:160:4: error: call to '__bad_copy_to' declared with attribute error: copy destination size is too small
	 160 |    __bad_copy_to();
	 |    ^~~~~~~~~~~~~~~
	 */
	pr_info("out-of-bounds in copy_from_user()\n");
	unused = copy_from_user(kmem, usermem, size + 1 + OOB_TAG_OFF);

	// similar gcc o/p as above...
	pr_info("out-of-bounds in copy_to_user()\n");
	unused = copy_to_user(usermem, kmem, size + 1 + OOB_TAG_OFF);
#endif

	pr_info("out-of-bounds in __copy_from_user()\n");
	unused = __copy_from_user(kmem, usermem, size + 1 + OOB_TAG_OFF);

	pr_info("out-of-bounds in __copy_to_user()\n");
	unused = __copy_to_user(usermem, kmem, size + 1 + OOB_TAG_OFF);

	pr_info("out-of-bounds in __copy_from_user_inatomic()\n");
	unused = __copy_from_user_inatomic(kmem, usermem, size + 1 + OOB_TAG_OFF);

	pr_info("out-of-bounds in __copy_to_user_inatomic()\n");
	unused = __copy_to_user_inatomic(usermem, kmem, size + 1 + OOB_TAG_OFF);

	pr_info("out-of-bounds in strncpy_from_user()\n");
	unused = strncpy_from_user(kmem, usermem, size + 1 + OOB_TAG_OFF);

	vm_munmap((unsigned long)usermem, PAGE_SIZE);
	kfree(kmem);
}

static int __init kmembugs_test_init(void)
{
	int stat;

	/*
	 * Realize that the kasan_save_enable_multi_shot() / kasan_restore_multi_shot()
	 * pair of functions work only on a kernel that has CONFIG_KASAN=y. Also,
	 * we're expecting Generic KASAN enabled.
	 */
#ifdef CONFIG_KASAN_GENERIC
	kasan_multishot = kasan_save_enable_multi_shot();
	pr_info("KASAN configured\n");
#else
	pr_info("KASAN NOT configured\n");
#endif
	if (IS_ENABLED(CONFIG_UBSAN))
		pr_info("UBSAN configured\n");
	else
		pr_info("UBSAN NOT configured\n");

	stat = debugfs_simple_intf_init();
	if (stat < 0)
		return stat;

	return 0;		/* success */
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

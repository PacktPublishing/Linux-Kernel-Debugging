/*
 * ch7/kmembugs_test/debugfs_kmembugs.c
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
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#else
#include <asm/uaccess.h>
#endif

//----------------- The testcase function prototypes, in order
int umr(void);                       // testcase 1
void *uar(void);                     // testcase 2
void leak_simple1(void);             // testcase 3.1
void *leak_simple2(void);            // testcase 3.2

int global_mem_oob_right(int mode, char *p);  // testcase 4.1/5.1
int global_mem_oob_left(int mode, char *p);   // testcase 4.2/5.2
int dynamic_mem_oob_right(int mode); // testcase 4.3/5.3
int dynamic_mem_oob_left(int mode);  // testcase 4.4/5.4

int uaf(void);                       // testcase 6
int double_free(void);               // testcase 7

// UBSAN testcases 8.x
void test_ubsan_add_overflow(void);
void test_ubsan_sub_overflow(void);
void test_ubsan_mul_overflow(void);
void test_ubsan_negate_overflow(void);
void test_ubsan_divrem_overflow(void);
void test_ubsan_shift_out_of_bounds(void);
void test_ubsan_out_of_bounds(void);
void test_ubsan_load_invalid_value(void);
void test_ubsan_misaligned_access(void);
void test_ubsan_object_size_mismatch(void);

noinline void oob_copy_user_test(void); // testcase 9
int umr_slub(void); // SLUB debug testcase, testcase 10
//----------------------------------------------

struct dentry *gparent;
EXPORT_SYMBOL(gparent);

extern char global_arr1[], global_arr2[], global_arr3[];

#define MAXUPASS 5
static ssize_t dbgfs_run_testcase(struct file *filp, const char __user *ubuf, size_t count, loff_t *fpos)
{
	char udata[MAXUPASS];
	volatile char *res1 = NULL, *res2 = NULL;

	if (count > MAXUPASS) {
		pr_warn("too much data attempted to be passed from userspace to here\n");
		return -ENOSPC;
	}
	if (copy_from_user(udata, ubuf, count))
		return -EIO;
	udata[count-1]='\0';
	pr_debug("testcase to run: %s\n", udata);

	/* 
	 * Now udata contains the data passed from userspace - the testcase # to run
	 * (as a string)
	 */
	if (!strncmp(udata, "1", 2))
		umr();
	else if (!strncmp(udata, "2", 2)) {
		res1 = uar();
        pr_info("testcase 2: UAR: res1 = \"%s\"\n",
            res1 == NULL ? "<whoops, it's NULL; UAR!>" : (char *)res1);
	} else if (!strncmp(udata, "3.1", 4))
		leak_simple1();
	else if (!strncmp(udata, "3.2", 4)) {
		res2 = (char *)leak_simple2(); // caller's expected to free the memory!
        pr_info(" res2 = \"%s\"\n",
            res2 == NULL ? "<whoops, it's NULL; UAR!>" : (char *)res2);
        if (0)      // test: ensure it isn't freed by the caller
            kfree((char *)res2);
	} else if (!strncmp(udata, "4.1", 4))
		global_mem_oob_right(READ, global_arr2);
	else if (!strncmp(udata, "4.2", 4))
		global_mem_oob_right(WRITE, global_arr2);
	else if (!strncmp(udata, "4.3", 4))
		global_mem_oob_left(READ, global_arr2);
	else if (!strncmp(udata, "4.4", 4))
		global_mem_oob_left(WRITE, global_arr2);
	else if (!strncmp(udata, "5.1", 4))
		dynamic_mem_oob_right(READ);
	else if (!strncmp(udata, "5.2", 4))
		dynamic_mem_oob_right(WRITE);
	else if (!strncmp(udata, "5.3", 4))
		dynamic_mem_oob_left(READ);
	else if (!strncmp(udata, "5.4", 4))
		dynamic_mem_oob_left(WRITE);
	else if (!strncmp(udata, "6", 2))
		uaf();
	else if (!strncmp(udata, "7", 2))
		double_free();
	else if (!strncmp(udata, "8.1", 4))
		test_ubsan_add_overflow();
	else if (!strncmp(udata, "8.2", 4))
		test_ubsan_sub_overflow();
	else if (!strncmp(udata, "8.3", 4))
		test_ubsan_mul_overflow();
	else if (!strncmp(udata, "8.4", 4))
		test_ubsan_negate_overflow();
	else if (!strncmp(udata, "8.4", 4))
		test_ubsan_divrem_overflow();
	else if (!strncmp(udata, "8.5", 4))
	    test_ubsan_shift_out_of_bounds();
	else if (!strncmp(udata, "8.6", 4))
		test_ubsan_out_of_bounds();
	else if (!strncmp(udata, "8.7", 4))
		test_ubsan_load_invalid_value();
	else if (!strncmp(udata, "8.8", 4))
		test_ubsan_misaligned_access();
	else if (!strncmp(udata, "8.9", 4))
		test_ubsan_object_size_mismatch();
	else if (!strncmp(udata, "9", 2))
		oob_copy_user_test();
	else if (!strncmp(udata, "10", 3))
		umr_slub();
	else
		pr_warn("Invalid testcase # (%s) passed\n", udata);

	return count;
}

static struct file_operations dbgfs_fops = {
	.write = dbgfs_run_testcase,
};

int debugfs_simple_intf_init(void)
{
	int stat = 0;
	struct dentry *file1;

	if (!IS_ENABLED(CONFIG_DEBUG_FS)) {
		pr_warn("debugfs unsupported! Aborting ...\n");
		return -EINVAL;
	}

	/* 1. Create a dir under the debugfs mount point, whose name is the
	 * module name */
	gparent = debugfs_create_dir(KBUILD_MODNAME, NULL);
	if (!gparent) {
		pr_info("debugfs_create_dir failed, aborting...\n");
		stat = PTR_ERR(gparent);
		goto out_fail_1;
	}

	/* Create a generic write-only-as-root debugfs file; arrange for a callback
	 * function on write (via the classic file_operations structure).
	 */
#define DBGFS_FILE	"lkd_dbgfs_run_testcase"
	file1 =
	    debugfs_create_file(DBGFS_FILE, 0200, gparent, NULL, &dbgfs_fops);
	if (!file1) {
		pr_info("debugfs_create_file failed, aborting...\n");
		stat = PTR_ERR(file1);
		goto out_fail_2;
	}
	pr_debug("debugfs file 1 <debugfs_mountpt>/%s/%s created\n",
		 KBUILD_MODNAME, DBGFS_FILE);

	pr_info("debugfs entry initialized\n");
	return 0;

 out_fail_2:
	debugfs_remove_recursive(gparent);
 out_fail_1:
	return stat;
}

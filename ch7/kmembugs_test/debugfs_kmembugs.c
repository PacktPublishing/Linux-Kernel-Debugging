

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

// The testcase function prototypes, in order
int umr(void);
void *uar(void);
//void *uaf(void);

int static_mem_oob_right(int mode);
int static_mem_oob_left(int mode);
int dynamic_mem_oob_right(int mode);
int dynamic_mem_oob_left(int mode);

void leak_simple1(void);
void *leak_simple2(void);

struct dentry *gparent;

#define MAXUPASS 4
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
		static_mem_oob_right(READ);
	else if (!strncmp(udata, "4.2", 4))
		static_mem_oob_right(WRITE);
	else if (!strncmp(udata, "4.3", 4))
		static_mem_oob_left(READ);
	else if (!strncmp(udata, "4.4", 4))
		static_mem_oob_left(WRITE);
	else if (!strncmp(udata, "5.1", 4))
		dynamic_mem_oob_right(READ);
	else if (!strncmp(udata, "5.2", 4))
		dynamic_mem_oob_right(WRITE);
	else if (!strncmp(udata, "5.3", 4))
		dynamic_mem_oob_left(READ);
	else if (!strncmp(udata, "5.4", 4))
		dynamic_mem_oob_left(WRITE);
	/*
	else if (!strncmp(udata, "6", 2))
		double_free();
	*/
	else
		pr_warn("Invalid testcase # (%s) passed\n", udata);

	return count;
}

static struct file_operations dbgfs_fops = {
	//.read = dbgfs_genread,
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

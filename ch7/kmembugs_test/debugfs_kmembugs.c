

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>

// The testcase function prototypes
int umr(void);

struct dentry *gparent;

#define MAXUPASS 4 // careful- k stack is small!
static ssize_t dbgfs_run_testcase(struct file *filp, const char __user *ubuf, size_t count, loff_t *fpos)
{
	char udata[MAXUPASS];

	if (count > MAXUPASS) {
		pr_warn("too much data attempted to be passed from userspace to here\n");
		return -ENOSPC;
	}
	if (copy_from_user(udata, ubuf, count))
		return -EIO;
	udata[count-1]='\0';
	pr_debug("user passed %zu bytes: %s\n", count, udata);

	if (!strncmp(udata, "1", 2))
		umr();
	else if (!strncmp(udata, "2", 2))
		pr_info("run tc 2\n");
	else if (!strncmp(udata, "4.3", 4))
		pr_info("run tc 4.3\n");

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

	/* Generic debugfs file + passing a pointer to a data structure as a
	 * demo.. the 4th param is a generic void * ptr; it's contents will be
	 * stored into the i_private field of the file's inode.
	 */
#define DBGFS_FILE1	"lkd_dbgfs_run_testcase"
	file1 =
	    debugfs_create_file(DBGFS_FILE1, 0640, gparent, NULL, &dbgfs_fops);
	if (!file1) {
		pr_info("debugfs_create_file failed, aborting...\n");
		stat = PTR_ERR(file1);
		goto out_fail_2;
	}
	pr_debug("debugfs file 1 <debugfs_mountpt>/%s/%s created\n",
		 KBUILD_MODNAME, DBGFS_FILE1);

#if 0
	/* 3. Create the debugfs file for the debug_level global; we use the
	 * helper routine to make it simple! There is a downside: we have no
	 * chance to perform a validity check on the value being written..
	 */
#define DBGFS_FILE2	"llkd_dbgfs_debug_level"
	debugfs_create_u32(DBGFS_FILE2, 0644, gparent, &debug_level);
	/*if (!file2) {
		pr_info("debugfs_create_u32 failed, aborting...\n");
		stat = PTR_ERR(file2);
		goto out_fail_3;
	}*/
	pr_debug("debugfs file 2 <debugfs_mountpt>/%s/%s created\n", KBUILD_MODNAME, DBGFS_FILE2);
#endif
	pr_info("debugfs entry initialized\n");
	return 0;

 out_fail_2:
	debugfs_remove_recursive(gparent);
 out_fail_1:
	return stat;
}

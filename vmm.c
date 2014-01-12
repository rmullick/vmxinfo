/*
 * A simple kernel module to explore various IA32_VMX_BASIC related values
 * Values are exported via /proc/vmxstat
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define	BIT48	16
#define	BIT49	17
#define BIT44	13

struct vmx_info {
	u32 revid;
	u32 lenvmxon;
	u32 size, width;	
	unsigned char memtype;
} vmx_info;

/* IA32_VMX_BASIC:32-44: contains # of bytes should allocate for VMXON and VMCS region
 * @bit44=1, then 32-43 are 0. @bit48: width of physical addr used for VMXON, VMCS and other
 * data structures referenced by pointers in VMCS.
 */
unsigned int low, high;

#define	IA32_VMX_BASIC	0x480

static int show_stat(struct seq_file *seq, void *data)
{
	seq_printf(seq, "revision id:%08x \n", vmx_info.revid);
	seq_printf(seq, "lenvmxon: %u\n", vmx_info.lenvmxon);
	seq_printf(seq, "memtype: %d\n", vmx_info.memtype);
	return 0;
}

static void *schedstat_start(struct seq_file *file, loff_t *offset)
{
	unsigned long n = *offset;
	if (n == 0)
		return (void *)1;
	else
		return NULL;
}

static void *schedstat_next(struct seq_file *file, void *data, loff_t *offset)
{
	(*offset)++;
	return schedstat_start(file, offset);
}

static void *schedstat_stop(struct seq_file *file, void *data, loff_t *offset)
{
	return NULL;
}

static const struct seq_operations schedstat_sops = {
	.start = schedstat_start,
	.next = schedstat_next,
	.stop = schedstat_stop,
	.show = show_stat,
};

static int schedstat_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &schedstat_sops);
}

static const struct file_operations proc_ops = {
	.open = schedstat_open, 
	.read = seq_read,
	.llseek = seq_lseek,
};

static void read_vmx_msr(void)
{
	asm volatile (  "rdmsr\n"
			:"=a"(vmx_info.revid), "=d"(high):"c"(IA32_VMX_BASIC):);

	if (test_bit(BIT44, (unsigned long*)&high))
		printk(KERN_INFO "bit44 is set!\n");
	else
		vmx_info.lenvmxon = high & 0x1fff;

	if (!test_bit(BIT48, (unsigned long *)&high))
		printk(KERN_INFO "bit48 is off.\n");

	if (test_bit(BIT49, (unsigned long*)&high))
		printk(KERN_INFO "processor supports vcpu dual-monitor.\n");

	vmx_info.memtype = (high >> 18) & 15;
}

static struct proc_dir_entry *test = NULL;

static int __init my_init(void)
{
	printk(KERN_INFO "Module loaded.\n");
	read_vmx_msr();
	test = proc_create("vmxstat", 0444, NULL, &proc_ops);
	return 0;
}

static void __exit my_exit(void)
{
	proc_remove(test);
	printk(KERN_INFO "Module unloading\n");
}

module_init(my_init)
module_exit(my_exit)
MODULE_LICENSE("GPL");
MODULE_AUTHOR("rmullick");

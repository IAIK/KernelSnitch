
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/posix-timers.h>
#include <linux/hashtable.h>
#include <linux/rhashtable.h>
#include <linux/ipc_namespace.h>
#include <linux/futex.h>
#include <linux/mm_types.h>
#include <linux/rtmutex.h>
#include <linux/namei.h>
#include <linux/fs_struct.h>
#include <linux/path.h>
#include <linux/hash.h>
#include <asm/io.h>
#include <asm/word-at-a-time.h>
#include <linux/msg.h>
#include <linux/sched/mm.h>
#include <linux/version.h>
#include "helper.h"

#define DEBUG_LVL 3

#if LINUX_VERSION_CODE > KERNEL_VERSION(6, 6, 0)
#define plist_for_each_entry_safe(pos, n, head, m)	\
	list_for_each_entry_safe(pos, n, &(head)->node_list, m.node_list)
#endif

#if DEBUG_LVL > 3
#define debug_debug(FMT, ...) do {printk(KERN_DEBUG "[%-5.5s%5u] " FMT, current->comm, current->pid, ##__VA_ARGS__);} while (0)
#else
#define debug_debug(FMT, ...) do {} while (0)
#endif

#if DEBUG_LVL > 2
#define debug_info(FMT, ...) do {printk(KERN_INFO "[%-5.5s%5u] " FMT, current->comm, current->pid, ##__VA_ARGS__);} while (0)
#else
#define debug_info(FMT, ...) do {} while (0)
#endif

#if DEBUG_LVL > 1
#define debug_alert(FMT, ...) do {printk(KERN_ALERT "[%-5.5s%5u] " FMT, current->comm, current->pid, ##__VA_ARGS__);} while (0)
#else
#define debug_alert(FMT, ...) do {} while (0)
#endif

#if DEBUG_LVL > 0
#define debug_error(FMT, ...) do {printk(KERN_ERR "[%-5.5s%5u] " FMT, current->comm, current->pid, ##__VA_ARGS__);} while (0)
#else
#define debug_error(FMT, ...) do {} while (0)
#endif

#define DEVICE_NAME "helper"
#define CLASS_NAME "helperclass"

int helper_init_device_driver(void);
static int helper_init(void);
static void helper_cleanup(void);
int helper_open(struct inode *inode, struct file *filp);
int helper_release(struct inode *inode, struct file *filep);
long helper_ioctl(struct file *file, unsigned int num, long unsigned int param);

static struct file_operations helper_fops = {
	.owner = THIS_MODULE,
	.open = helper_open,
	.release = helper_release,
	.unlocked_ioctl = helper_ioctl,
};

static int helper_major;
static struct class *helper_class;
static struct device *helper_device;

static struct hlist_head *posix_timers_hashtable;
struct futex_hash_bucket;
union futex_key;
static struct futex_hash_bucket *(*futex_hash)(union futex_key *key);
struct msg_queue;
struct msg_queue *(*ipc_obtain_object_check)(struct ipc_ids *ns, int id);
static struct kprobe kp = {
	.symbol_name = "posix_timers_hashtable"
};
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
static struct kprobe kp1 = {
	.symbol_name = "futex_hash"
};
#else
static struct kprobe kp1 = {
	.symbol_name = "hash_futex"
};
#endif
static struct kprobe kp2 = {
	.symbol_name = "__futex_data"
};
static struct kprobe kp3 = {
	.symbol_name = "ipc_obtain_object_check"
};
struct futex_data {
	struct futex_hash_bucket *queues;
	unsigned long            hashsize;
} __aligned(2*sizeof(long));
struct futex_data *__futex_data;
#define futex_queues   (__futex_data->queues)
#define futex_hashsize (__futex_data->hashsize)

/*
 * Initialization device driver
 */
int helper_init_device_driver(void)
{
	int ret;
	debug_info("helper:init_device_driver: start\n");

	ret = register_chrdev(0, DEVICE_NAME, &helper_fops);
	if (ret < 0) goto ERROR;
	helper_major = ret;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 4, 0)
	helper_class = class_create(THIS_MODULE, CLASS_NAME);
#else
	helper_class = class_create(CLASS_NAME);
#endif
	if (IS_ERR(helper_class)) {
		ret = PTR_ERR(helper_class);
		goto ERROR1;
	}

	helper_device = device_create(helper_class, 0, MKDEV(helper_major, 0), 0, DEVICE_NAME);
	if (IS_ERR(helper_device)) {
		ret = PTR_ERR(helper_device);
		goto ERROR2;
	}

	debug_info("helper:init_device_driver: done '/dev/%s c %d' 0 created\n", DEVICE_NAME, helper_major);
	return 0;

ERROR2:
	debug_alert("helper:init_device_driver: class destroy\n");
	class_unregister(helper_class);
	class_destroy(helper_class);
ERROR1:
	debug_alert("helper:init_device_driver: unregister chrdev\n");
	unregister_chrdev(helper_major, CLASS_NAME);
ERROR:
	debug_alert("helper:init_device_driver: fail %d\n", ret);
	helper_device = 0;
	helper_class = 0;
	helper_major = -1;
	return ret;
}

/*
 * Initialization
 */
static int helper_init(void)
{
	int ret;
	debug_info("helper:init: start with kernel version: %d.%d.%d\n",
           (LINUX_VERSION_CODE >> 16) & 0xFF,   // Major version
           (LINUX_VERSION_CODE >> 8) & 0xFF,    // Minor version
           LINUX_VERSION_CODE & 0xFF);  

	ret = helper_init_device_driver();
	if (ret) goto ERROR;

    register_kprobe(&kp);
	posix_timers_hashtable = (void *)kp.addr;
    register_kprobe(&kp1);
	futex_hash = (struct futex_hash_bucket *(*)(union futex_key *))kp1.addr;
    register_kprobe(&kp2);
	__futex_data = (struct futex_data *)kp2.addr;
	register_kprobe(&kp3);
	ipc_obtain_object_check = (struct msg_queue *(*)(struct ipc_ids *, int ))kp3.addr;

	debug_info("helper:init: posix_timers_hashtable  %016zx\n", (size_t)posix_timers_hashtable);
	debug_info("helper:init: futex_hash              %016zx\n", (size_t)futex_hash);
	debug_info("helper:init: __futex_data            %016zx\n", (size_t)__futex_data);
	debug_info("helper:init: ipc_obtain_object_check %016zx\n", (size_t)ipc_obtain_object_check);

	debug_info("helper:init: done\n");
	return 0;

ERROR:
	debug_alert("helper:init: error\n");
	return ret;
}

/*
 * Cleanup
 */
static void helper_cleanup(void)
{
	debug_info("helper:cleanup\n");
    unregister_kprobe(&kp);
    unregister_kprobe(&kp1);
    unregister_kprobe(&kp2);
    unregister_kprobe(&kp3);
	device_destroy(helper_class, MKDEV(helper_major, 0));
	class_unregister(helper_class);
	class_destroy(helper_class);
	unregister_chrdev(helper_major, DEVICE_NAME);
}

/*
 * Close "/dev/helper"
 */
int helper_release(struct inode *inode, struct file *filep)
{
	debug_info("helper:release\n");
	module_put(THIS_MODULE);
	return 0;
}
EXPORT_SYMBOL(helper_release);

/*
 * Open "/dev/helper"
 */
int helper_open(struct inode *inode, struct file *filp)
{
	debug_info("helper:open\n");
	try_module_get(THIS_MODULE);
	return 0;
}
EXPORT_SYMBOL(helper_open);

/*
 * Getting the count (ground truth) for ipc_ids.key_ht
 */
#define IPC_MSG_IDS	1
#define msg_ids(ns)	((ns)->ids[IPC_MSG_IDS])
static const struct rhashtable_params ipc_kht_params = {
	.head_offset		= offsetof(struct kern_ipc_perm, khtnode),
	.key_offset		= offsetof(struct kern_ipc_perm, key),
	.key_len		= sizeof_field(struct kern_ipc_perm, key),
	.automatic_shrinking	= true,
};
static long ___rhashtable_lookup(
	struct rhashtable *ht, const void *key,
	const struct rhashtable_params params)
{
	struct rhashtable_compare_arg arg = {
		.ht = ht,
		.key = key,
	};
	struct rhash_lock_head __rcu *const *bkt;
	struct bucket_table *tbl;
	struct rhash_head *he;
	unsigned int hash;
	long count = 0;

	tbl = rht_dereference_rcu(ht->tbl, ht);
restart:
	hash = rht_key_hashfn(ht, tbl, key, params);
	bkt = rht_bucket(tbl, hash);
	do {
		rht_for_each_rcu_from(he, rht_ptr_rcu(bkt), tbl, hash) {
			if (params.obj_cmpfn ?
			    params.obj_cmpfn(&arg, rht_obj(ht, he)) :
			    rhashtable_compare(&arg, rht_obj(ht, he))) {
				count++;
				continue;
			}
			return count;
		}
		/* An object might have been moved to a different hash chain,
		 * while we walk along it - better check and retry.
		 */
	} while (he != RHT_NULLS_MARKER(bkt));

	/* Ensure we see any new tables. */
	smp_rmb();

	tbl = rht_dereference_rcu(tbl->future_tbl, ht);
	if (unlikely(tbl))
		goto restart;

	return count;
}
static long _ipc_findkey(struct ipc_ids *ids, key_t key)
{
	return ___rhashtable_lookup(&ids->key_ht, &key,
					      ipc_kht_params);
}
static long _ksys_msgget(key_t key)
{
	return _ipc_findkey(&msg_ids(current->nsproxy->ipc_ns), key);
}

/*
 * Hash for k_itimer
 */
static int hash(struct signal_struct *sig, unsigned int nr)
{
	return hash_32(hash32_ptr(sig) ^ nr, 9);
}

struct futex_hash_bucket {
	atomic_t waiters;
	spinlock_t lock;
	struct plist_head chain;
} ____cacheline_aligned_in_smp;
struct futex_pi_state {
	struct list_head list;
	struct rt_mutex_base pi_mutex;
	struct task_struct *owner;
	refcount_t refcount;
	union futex_key key;
} __randomize_layout;
struct futex_q {
	struct plist_node list;
	struct task_struct *task;
	spinlock_t *lock_ptr;
	void *wake;
	void *wake_data;
	union futex_key key;
	struct futex_pi_state *pi_state;
	struct rt_mutex_waiter *rt_waiter;
	union futex_key *requeue_pi_key;
	u32 bitset;
	atomic_t requeue_state;
	struct rcuwait requeue_wait;
} __randomize_layout;
static size_t futex_read_count(size_t futex_addr, struct mm_struct *mm)
{
	union futex_key key;
	struct futex_hash_bucket *hb;
	size_t count = 0;
	struct futex_q *this;
	struct futex_q *next;

	key.private.mm = mm;
	key.private.address = futex_addr & ~0xfff;
	key.private.offset = futex_addr & 0xfff;
	hb = futex_hash(&key);
	spin_lock(&hb->lock);
	plist_for_each_entry_safe(this, next, &hb->chain, list) {
		count++;
	}
	spin_unlock(&hb->lock);
	return count;
}

struct msg_queue {
	struct kern_ipc_perm q_perm;
	time64_t q_stime;
	time64_t q_rtime;
	time64_t q_ctime;
	unsigned long q_cbytes;
	unsigned long q_qnum;
	unsigned long q_qbytes;
	struct pid *q_lspid;
	struct pid *q_lrpid;
	struct list_head q_messages;
	struct list_head q_receivers;
	struct list_head q_senders;
};
#define msg_ids(ns)	((ns)->ids[IPC_MSG_IDS])

/*
 * ioctl code
 */
long helper_ioctl(struct file *_, unsigned int num, long unsigned int param)
{
	long ret;
	msg_t msg;
	size_t *uaddr = 0;
	size_t tmp = -1;
	struct hlist_head *head;
	struct k_itimer *timer;
	size_t count = 0;
	debug_debug("helper:ioctl: start num 0x%08x param 0x%016lx\n", num, param);

	ret = copy_from_user((msg_t*)&msg, (msg_t*)param, sizeof(msg_t));
	if (ret < 0) {
		debug_alert("helper:ioctl: copy_from_user failed\n");
		ret = -1;
		goto RETURN;
	}

	switch (num) {
		case TIMER_READ_CNT_HASH:
			head = &posix_timers_hashtable[hash(current->signal, msg.timer_rd.timer_id)];
			hlist_for_each_entry(timer, head, t_hash) {
				count++;
			}
			tmp = count;
			uaddr = (size_t *)msg.timer_rd.uaddr;
			debug_debug("helper:ioctl: timer with %zd has %zd entries\n", msg.timer_rd.timer_id, count);
			goto COPY_TMP_TO_USER;

		case TIMER_READ_INDEX_HASH:
			tmp = hash(current->signal, msg.timer_rd.timer_id);
			uaddr = (size_t *)msg.timer_rd.uaddr;
			debug_debug("helper:ioctl: hash for %zd is %zx\n", msg.timer_rd.timer_id, tmp);
			goto COPY_TMP_TO_USER;

		case KEY_READ_CNT:
			uaddr = (size_t *)msg.timer_rd.uaddr;
			tmp = _ksys_msgget(msg.key_rd.key_id);
			debug_debug("helper:ioctl: key with %zd has %zd entries\n", msg.key_rd.key_id, tmp);
			goto COPY_TMP_TO_USER;

		case FUTEX_READ_CNT:
			tmp = futex_read_count(msg.futex_rd.futex_addr, current->mm);
			uaddr = (size_t *)msg.futex_rd.uaddr;
			debug_debug("helper:ioctl: futex with address %zd has %zd entries\n", msg.futex_rd.futex_addr, tmp);
			goto COPY_TMP_TO_USER;

		default:
			debug_alert("helper:ioctl: no valid num\n");
			ret = -1;
			goto RETURN;
	}
	ret = 0;
	goto DONE;
COPY_TMP_TO_USER:
	BUG_ON(uaddr == 0 && "forgot to set uaddr");
	BUG_ON(tmp == -1 && "forgot to set tmp");
	debug_debug("helper:ioctl: copy 0x%016zx to mem[0x%016zx]\n", tmp, (size_t)uaddr);
	ret = copy_to_user(uaddr, &tmp, sizeof(size_t));
	if (ret < 0) {
		debug_alert("helper:ioctl: copy_to_user failed\n");
		ret = -1;
		goto RETURN;
	}
	ret = 0;

DONE:
	debug_debug("helper:ioctl: done\n");
RETURN:
	return ret;
}
EXPORT_SYMBOL(helper_ioctl);

module_init(helper_init);
module_exit(helper_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lukas Maar");
MODULE_DESCRIPTION("LKM");
MODULE_VERSION("0.1");

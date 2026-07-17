/* CTF|认证 */
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/pid.h>
#include <linux/sched/mm.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include "../shared/delta_protocol.h"

static DEFINE_MUTEX(delta_registration_lock);
static atomic_t delta_open_count = ATOMIC_INIT(0);
static bool delta_registered;
static bool delta_stopping;
static void delta_detach_worker(struct work_struct *work);
static void delta_reattach_worker(struct work_struct *work);
static DECLARE_DELAYED_WORK(delta_detach_work, delta_detach_worker);
static DECLARE_DELAYED_WORK(delta_reattach_work, delta_reattach_worker);

static int delta_open(struct inode *inode, struct file *file)
{
    (void)inode;
    (void)file;
    atomic_inc(&delta_open_count);
    return 0;
}

static int delta_release(struct inode *inode, struct file *file)
{
    (void)inode;
    (void)file;
    if (atomic_dec_and_test(&delta_open_count) && !READ_ONCE(delta_stopping))
        schedule_delayed_work(&delta_reattach_work, msecs_to_jiffies(250));
    return 0;
}

static long delta_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    void __user *user = (void __user *)arg;
    (void)file;

    switch (cmd) {
    case DELTA_IOCTL_GET_VERSION: {
        __u16 version = DELTA_PROTOCOL_VERSION;
        return copy_to_user(user, &version, sizeof(version)) ? -EFAULT : 0;
    }
    case DELTA_IOCTL_GET_CAPABILITIES: {
        struct delta_capabilities caps = {
            .protocol_version = DELTA_PROTOCOL_VERSION,
            .flags = DELTA_CAP_PROCESS_READ,
            .max_read_size = DELTA_MAX_READ_SIZE,
        };
        return copy_to_user(user, &caps, sizeof(caps)) ? -EFAULT : 0;
    }
    case DELTA_IOCTL_PING: {
        struct delta_ping_packet packet;
        if (copy_from_user(&packet, user, sizeof(packet)))
            return -EFAULT;
        if (packet.header.magic != DELTA_PROTOCOL_MAGIC ||
            packet.header.version != DELTA_PROTOCOL_VERSION ||
            packet.header.size != sizeof(packet))
            return -EPROTO;
        packet.status = 0;
        if (copy_to_user(user, &packet, sizeof(packet)))
            return -EFAULT;
        /* Detach the discovery endpoint after a verified client owns an fd. */
        schedule_delayed_work(&delta_detach_work, msecs_to_jiffies(50));
        return 0;
    }
    case DELTA_IOCTL_READ_PROCESS: {
        struct delta_read_request request;
        struct pid *kernel_pid;
        struct task_struct *task;
        int copied;

        if (copy_from_user(&request, user, sizeof(request)))
            return -EFAULT;
        request.status = -EINVAL;
        request.transferred = 0;
        if (request.pid <= 0 || request.size == 0 || request.size > DELTA_MAX_READ_SIZE)
            goto read_done;

        kernel_pid = find_get_pid(request.pid);
        if (!kernel_pid) {
            request.status = -ESRCH;
            goto read_done;
        }
        task = get_pid_task(kernel_pid, PIDTYPE_PID);
        put_pid(kernel_pid);
        if (!task) {
            request.status = -ESRCH;
            goto read_done;
        }

        copied = access_process_vm(task, (unsigned long)request.address,
                                   request.data, request.size, 0);
        put_task_struct(task);
        if (copied < 0) {
            request.status = copied;
        } else {
            request.transferred = copied;
            request.status = copied == request.size ? 0 : -EFAULT;
        }
read_done:
        return copy_to_user(user, &request, sizeof(request)) ? -EFAULT : 0;
    }
    default:
        return -ENOTTY;
    }
}

static const struct file_operations delta_fops = {
    .owner = THIS_MODULE,
    .open = delta_open,
    .release = delta_release,
    .unlocked_ioctl = delta_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = delta_ioctl,
#endif
};

static struct miscdevice delta_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "delta_kernel",
    .fops = &delta_fops,
    .mode = 0600,
};

static void delta_detach_worker(struct work_struct *work)
{
    (void)work;
    mutex_lock(&delta_registration_lock);
    if (delta_registered && atomic_read(&delta_open_count) > 0 && !delta_stopping) {
        misc_deregister(&delta_device);
        delta_registered = false;
    }
    mutex_unlock(&delta_registration_lock);
}

static void delta_reattach_worker(struct work_struct *work)
{
    int ret;
    (void)work;
    mutex_lock(&delta_registration_lock);
    if (!delta_registered && atomic_read(&delta_open_count) == 0 && !delta_stopping) {
        ret = misc_register(&delta_device);
        if (!ret)
            delta_registered = true;
    }
    mutex_unlock(&delta_registration_lock);
}

static int __init delta_init(void)
{
    int ret = misc_register(&delta_device);
    if (!ret)
        delta_registered = true;
    return ret;
}

static void __exit delta_exit(void)
{
    WRITE_ONCE(delta_stopping, true);
    cancel_delayed_work_sync(&delta_detach_work);
    cancel_delayed_work_sync(&delta_reattach_work);
    mutex_lock(&delta_registration_lock);
    if (delta_registered) {
        misc_deregister(&delta_device);
        delta_registered = false;
    }
    mutex_unlock(&delta_registration_lock);
}

module_init(delta_init);
module_exit(delta_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Delta Research");
MODULE_DESCRIPTION("Delta Android kernel communication module");
MODULE_VERSION("0.1.0");


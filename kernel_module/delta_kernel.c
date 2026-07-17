/* CTF|认证 */
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/pid.h>
#include <linux/sched/mm.h>
#include <linux/uaccess.h>
#include "../shared/delta_protocol.h"

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
        return copy_to_user(user, &packet, sizeof(packet)) ? -EFAULT : 0;
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

static int __init delta_init(void)
{
    int ret = misc_register(&delta_device);
    if (!ret)
        pr_info("delta_kernel: CTF|认证 loaded protocol=%u\n", DELTA_PROTOCOL_VERSION);
    return ret;
}

static void __exit delta_exit(void)
{
    misc_deregister(&delta_device);
    pr_info("delta_kernel: unloaded\n");
}

module_init(delta_init);
module_exit(delta_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Delta Research");
MODULE_DESCRIPTION("Delta Android kernel communication module");
MODULE_VERSION("0.1.0");


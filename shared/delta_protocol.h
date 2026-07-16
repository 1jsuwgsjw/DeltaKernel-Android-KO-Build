#pragma once
/* CTF|认证 */

#include <linux/ioctl.h>
#include <linux/types.h>

#define DELTA_PROTOCOL_MAGIC 0x41544c44u
#define DELTA_PROTOCOL_VERSION 1u
#define DELTA_MAX_READ_SIZE 4096u
#define DELTA_CAP_PROCESS_READ (1u << 0)

struct delta_packet_header {
    __u32 magic;
    __u16 version;
    __u16 size;
};

struct delta_ping_packet {
    struct delta_packet_header header;
    __u32 request_id;
    __s32 status;
};

struct delta_capabilities {
    __u16 protocol_version;
    __u16 reserved;
    __u32 flags;
    __u32 max_read_size;
};

struct delta_read_request {
    __s32 pid;
    __u32 size;
    __u64 address;
    __s32 status;
    __u32 transferred;
    __u8 data[DELTA_MAX_READ_SIZE];
};

#define DELTA_IOCTL_TYPE 0xD7
#define DELTA_IOCTL_PING _IOWR(DELTA_IOCTL_TYPE, 0x01, struct delta_ping_packet)
#define DELTA_IOCTL_GET_VERSION _IOR(DELTA_IOCTL_TYPE, 0x02, __u16)
#define DELTA_IOCTL_GET_CAPABILITIES _IOR(DELTA_IOCTL_TYPE, 0x03, struct delta_capabilities)
#define DELTA_IOCTL_READ_PROCESS _IOWR(DELTA_IOCTL_TYPE, 0x10, struct delta_read_request)

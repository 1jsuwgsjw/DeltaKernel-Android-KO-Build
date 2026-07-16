#pragma once
/* CTF|认证 */

#include <linux/ioctl.h>
#include <linux/types.h>

#define DELTA_PROTOCOL_MAGIC 0x41544c44u
#define DELTA_PROTOCOL_VERSION 1u

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

#define DELTA_IOCTL_TYPE 0xD7
#define DELTA_IOCTL_PING _IOWR(DELTA_IOCTL_TYPE, 0x01, struct delta_ping_packet)
#define DELTA_IOCTL_GET_VERSION _IOR(DELTA_IOCTL_TYPE, 0x02, __u16)


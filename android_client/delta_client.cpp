// CTF|认证
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../shared/delta_protocol.h"

int main()
{
    int fd = open("/dev/delta_kernel", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        std::fprintf(stderr, "open: %s\n", std::strerror(errno));
        return 1;
    }
    __u16 version = 0;
    if (ioctl(fd, DELTA_IOCTL_GET_VERSION, &version) != 0) {
        std::fprintf(stderr, "GET_VERSION: %s\n", std::strerror(errno));
        close(fd);
        return 2;
    }
    delta_ping_packet packet{{DELTA_PROTOCOL_MAGIC, DELTA_PROTOCOL_VERSION,
                              sizeof(delta_ping_packet)}, 0x20260717u, -1};
    if (ioctl(fd, DELTA_IOCTL_PING, &packet) != 0 || packet.status != 0) {
        std::fprintf(stderr, "PING: %s\n", std::strerror(errno));
        close(fd);
        return 3;
    }
    std::printf("CTF|认证 connected protocol=%u request=%u\n", version, packet.request_id);
    close(fd);
    return 0;
}


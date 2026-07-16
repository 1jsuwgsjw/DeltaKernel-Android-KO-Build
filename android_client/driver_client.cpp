// CTF|认证
#include "driver_client.hpp"
#include "../shared/delta_protocol.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

DriverClient::~DriverClient() { close(); }
bool DriverClient::connect(const char* path) {
    close(); fd_ = open(path, O_RDWR | O_CLOEXEC);
    if (fd_ < 0) { error_ = std::strerror(errno); return false; }
    return ping(0x20260717u);
}
void DriverClient::close() { if (fd_ >= 0) ::close(fd_); fd_ = -1; }
bool DriverClient::ping(std::uint32_t id) {
    delta_ping_packet p{{DELTA_PROTOCOL_MAGIC, DELTA_PROTOCOL_VERSION, sizeof(p)}, id, -1};
    if (ioctl(fd_, DELTA_IOCTL_PING, &p) || p.status) { error_ = std::strerror(errno); return false; }
    return true;
}
bool DriverClient::capabilities(std::uint32_t& flags, std::uint32_t& max_read) {
    delta_capabilities c{};
    if (ioctl(fd_, DELTA_IOCTL_GET_CAPABILITIES, &c)) { error_ = std::strerror(errno); return false; }
    flags = c.flags; max_read = c.max_read_size; return true;
}
bool DriverClient::read_process(std::int32_t pid, std::uint64_t address, void* out, std::size_t size) {
    if (!out || !size || size > DELTA_MAX_READ_SIZE) return false;
    delta_read_request r{}; r.pid = pid; r.address = address; r.size = static_cast<__u32>(size);
    if (ioctl(fd_, DELTA_IOCTL_READ_PROCESS, &r)) { error_ = std::strerror(errno); return false; }
    if (r.status || r.transferred != size) { error_ = "kernel read failed: " + std::to_string(r.status); return false; }
    std::memcpy(out, r.data, size); return true;
}


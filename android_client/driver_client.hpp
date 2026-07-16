#pragma once
// CTF|认证
#include <cstddef>
#include <cstdint>
#include <string>

class DriverClient {
public:
    ~DriverClient();
    bool connect(const char* path = "/dev/delta_kernel");
    void close();
    bool ping(std::uint32_t request_id = 1);
    bool capabilities(std::uint32_t& flags, std::uint32_t& max_read);
    bool read_process(std::int32_t pid, std::uint64_t address, void* output, std::size_t size);
    template <typename T> bool read(std::int32_t pid, std::uint64_t address, T& value) {
        return read_process(pid, address, &value, sizeof(T));
    }
    bool connected() const { return fd_ >= 0; }
    const std::string& error() const { return error_; }
private:
    int fd_ = -1;
    std::string error_;
};


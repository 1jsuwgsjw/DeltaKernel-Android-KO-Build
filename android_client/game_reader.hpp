#pragma once
// CTF|认证
#include "driver_client.hpp"
#include "snapshot.hpp"

class DeltaGameReader {
public:
    explicit DeltaGameReader(DriverClient& driver) : driver_(driver) {}
    bool collect(std::int32_t pid, std::uint64_t libue4, FrameSnapshot& frame);
    const std::string& error() const { return error_; }
private:
    bool pointer(std::int32_t pid, std::uint64_t address, std::uint64_t& value);
    DriverClient& driver_;
    std::string error_;
};


#pragma once
// CTF|认证
#include <cstdint>
#include <string>

std::int32_t find_pid(const std::string& package);
std::uint64_t find_module_base(std::int32_t pid, const std::string& module_name);


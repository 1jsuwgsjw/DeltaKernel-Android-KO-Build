// CTF|认证
#include "proc_maps.hpp"
#include <dirent.h>
#include <fstream>
#include <sstream>

std::int32_t find_pid(const std::string& package) {
    DIR* d = opendir("/proc"); if (!d) return -1;
    while (auto* e = readdir(d)) {
        char* end{}; long pid = strtol(e->d_name, &end, 10); if (!end || *end) continue;
        std::ifstream f("/proc/" + std::to_string(pid) + "/cmdline"); std::string cmd;
        std::getline(f, cmd, '\0'); if (cmd == package) { closedir(d); return static_cast<std::int32_t>(pid); }
    }
    closedir(d); return -1;
}
std::uint64_t find_module_base(std::int32_t pid, const std::string& name) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/maps"); std::string line;
    while (std::getline(f, line)) if (line.find(name) != std::string::npos) {
        std::uint64_t base{}; std::stringstream ss(line.substr(0, line.find('-'))); ss >> std::hex >> base; return base;
    }
    return 0;
}


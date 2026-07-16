// CTF|认证
#include "driver_client.hpp"
#include "proc_maps.hpp"
#include "game_reader.hpp"
#include <cstdio>

int main() {
    DriverClient driver;
    if (!driver.connect()) { std::fprintf(stderr, "driver: %s\n", driver.error().c_str()); return 1; }
    auto pid = find_pid("com.tencent.tmgp.dfm");
    auto base = pid > 0 ? find_module_base(pid, "libUE4.so") : 0;
    std::uint32_t caps{}, max_read{}; driver.capabilities(caps, max_read);
    std::printf("CTF|认证 pid=%d libUE4=0x%llx caps=0x%x max_read=%u\n",
                pid, static_cast<unsigned long long>(base), caps, max_read);
    if (pid > 0 && base) {
        unsigned char elf[4]{};
        if (driver.read_process(pid, base, elf, sizeof(elf)))
            std::printf("ELF=%02x%02x%02x%02x\n", elf[0], elf[1], elf[2], elf[3]);
        FrameSnapshot frame{};
        DeltaGameReader reader(driver);
        if (reader.collect(pid, base, frame))
            std::printf("world snapshot entities=%zu sequence=%llu\n", frame.entities.size(),
                        static_cast<unsigned long long>(frame.sequence));
        else
            std::printf("snapshot pending: %s\n", reader.error().c_str());
    }
    return 0;
}

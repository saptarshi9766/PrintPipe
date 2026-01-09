#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace printpipe {

struct SpoolBuffer {
    std::vector<std::uint8_t> bytes;
    std::string mime = "application/octet-stream";
    std::chrono::steady_clock::time_point created_at = std::chrono::steady_clock::now();
};

} // namespace printpipe

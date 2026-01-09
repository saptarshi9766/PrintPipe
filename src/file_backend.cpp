/*


cd /home/smandal/Develop/build

# Core library only
make printpipe

# HTTP server library (includes core)
make printpipe_server

# HTTP server executable
make printpipe_http_server

# Demo application
make printpipe_demo

# Test suite
make printpipe_tests

..................................................
cd /home/smandal/Develop
rm -rf build
mkdir build && cd build
cmake ..
make -j$(nproc)



*/


#include "printpipe/file_backend.hpp"

#include <fstream>

#include "printpipe/job.hpp"

namespace printpipe {

FileBackend::FileBackend(std::filesystem::path out_dir)
    : out_dir_(std::move(out_dir)) {}

bool FileBackend::print(const Job& job, std::string_view payload) {
    try {
        std::filesystem::create_directories(out_dir_);
        auto path = out_dir_ / (job.name() + ".txt");

        std::ofstream out(path, std::ios::binary);
        if (!out) return false;

        out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
        out.flush();
        return static_cast<bool>(out);
    } catch (...) {
        return false;
    }
}

} // namespace printpipe

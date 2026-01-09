#pragma once

#include <filesystem>
#include <string_view>

#include "printpipe/backend.hpp"

namespace printpipe {

class FileBackend final : public IBackend {
public:
    explicit FileBackend(std::filesystem::path out_dir = "out");

    bool print(const Job& job, std::string_view payload) override;

private:
    std::filesystem::path out_dir_;
};

} // namespace printpipe

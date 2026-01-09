#pragma once

#include <string_view>

namespace printpipe {

class Job;

class IBackend {
public:
    virtual ~IBackend() = default;

    // "Print" the already-spooled payload of the job.
    virtual bool print(const Job& job, std::string_view payload) = 0;
};

} // namespace printpipe

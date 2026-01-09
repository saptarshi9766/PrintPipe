#pragma once

#include <chrono>
#include <string>

#include "printpipe/job.hpp"

namespace printpipe {

enum class EventKind {
    StateChanged,
    RejectedTransition
};

struct JobEvent {
    EventKind kind{};
    std::string job_name;
    JobState from{};
    JobState to{};
    std::chrono::steady_clock::time_point ts{};
    std::string reason;
};

} // namespace printpipe

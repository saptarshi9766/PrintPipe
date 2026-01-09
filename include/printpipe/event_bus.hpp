#pragma once

#include <mutex>
#include <vector>

#include "printpipe/events.hpp"

namespace printpipe {

class EventBus {
public:
    void publish(JobEvent ev) {
        std::lock_guard<std::mutex> lk(mu_);
        events_.push_back(std::move(ev));
    }

    std::vector<JobEvent> snapshot() const {
        std::lock_guard<std::mutex> lk(mu_);
        return events_;
    }

    void clear() {
        std::lock_guard<std::mutex> lk(mu_);
        events_.clear();
    }

private:
    mutable std::mutex mu_;
    std::vector<JobEvent> events_;
};

} // namespace printpipe

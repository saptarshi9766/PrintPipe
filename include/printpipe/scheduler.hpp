#pragma once

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

#include "printpipe/job.hpp"
#include "printpipe/spooler.hpp"
#include "printpipe/backend.hpp"


namespace printpipe {

class Scheduler {
public:
    Scheduler();
    ~Scheduler();

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    void start();
    void stop();

    bool submit(std::shared_ptr<Job> job);

    void set_spooler(SpoolerPtr s) { spooler_ = std::move(s); }
    void set_backend(std::shared_ptr<IBackend> b) { backend_ = std::move(b); }

private:
    void worker_loop();

    std::mutex mu_;
    std::condition_variable cv_;
    std::deque<std::shared_ptr<Job>> q_;

    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};

    SpoolerPtr spooler_;
    std::shared_ptr<IBackend> backend_;

};

} // namespace printpipe

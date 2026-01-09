#include <thread>

#include "printpipe/scheduler.hpp"

namespace printpipe {

Scheduler::Scheduler()
    : spooler_(std::make_shared<TextSpooler>()) {}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) return;

    stop_requested_.store(false);
    worker_ = std::thread([this] { worker_loop(); });
}

void Scheduler::stop() {
    if (!running_.exchange(false)) return;

    stop_requested_.store(true);
    cv_.notify_all();

    if (worker_.joinable()) worker_.join();

    std::lock_guard<std::mutex> lk(mu_);
    q_.clear();
}

bool Scheduler::submit(std::shared_ptr<Job> job) {
    if (!job) return false;
    if (stop_requested_.load()) return false;

    {
        std::lock_guard<std::mutex> lk(mu_);
        q_.push_back(std::move(job));
    }
    cv_.notify_one();
    return true;
}

void Scheduler::worker_loop() {
    while (!stop_requested_.load()) {
        std::shared_ptr<Job> job;

        {
            std::unique_lock<std::mutex> lk(mu_);
            cv_.wait(lk, [&] { return stop_requested_.load() || !q_.empty(); });

            if (stop_requested_.load()) break;
            job = std::move(q_.front());
            q_.pop_front();
        }

        if (!job) continue;

        if (job->state() == JobState::Created) {
            (void)job->enqueue();
        }

        if (!job->schedule())       continue;
        if (!job->start_spooling()) continue;

        // Step 2: spool some data (for now: text buffer)
        if (!spooler_) {
            if (!Job::is_terminal(job->state())) (void)job->fail();
            continue;
        }

        SpoolResult sp = spooler_->spool(*job);
        if (!sp.ok) {
            if (!Job::is_terminal(job->state())) (void)job->fail();
            continue;
        }
if (!job->start_printing()) continue;

// ---- Step 3: print via backend ----
bool ok = false;
if (backend_) {
    const auto payload = job->payload_copy();
    ok = backend_->print(*job, payload);
} else {
    // No backend configured => fail fast (keeps behavior explicit)
    ok = false;
}

if (!ok) {
    job->fail();
    continue;
}

// If it was canceled while printing, complete() will fail due to terminal state
job->complete();

    }
}

} // namespace printpipe

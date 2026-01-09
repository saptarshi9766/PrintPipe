#include "printpipe/job.hpp"
#include "printpipe/event_bus.hpp"

namespace printpipe {

Job::Job(std::string name)
    : name_(std::move(name)) {}

const std::string& Job::name() const noexcept {
    return name_;
}

JobState Job::state() const noexcept {
    return state_.load(std::memory_order_acquire);
}

bool Job::is_terminal(JobState s) noexcept {
    return s == JobState::Completed
        || s == JobState::Canceled
        || s == JobState::Failed;
}

bool Job::is_valid_transition(JobState from, JobState to) noexcept {
    if (from == to) return true;      // idempotent no-op
    if (is_terminal(from)) return false;

    switch (from) {
        case JobState::Created:
            return to == JobState::Queued
                || to == JobState::Canceled
                || to == JobState::Failed;

        case JobState::Queued:
            return to == JobState::Scheduled
                || to == JobState::Canceled
                || to == JobState::Failed;

        case JobState::Scheduled:
            return to == JobState::Spooling
                || to == JobState::Canceled
                || to == JobState::Failed;

        case JobState::Spooling:
            return to == JobState::Printing
                || to == JobState::Canceled
                || to == JobState::Failed;

        case JobState::Printing:
            return to == JobState::Completed
                || to == JobState::Canceled
                || to == JobState::Failed;

        case JobState::Completed:
        case JobState::Canceled:
        case JobState::Failed:
            return false;
    }
    return false;
}

TransitionResult Job::try_transition(JobState to) noexcept {
    JobState from = state();

    // ---- Reject invalid transitions early ----
    if (!is_valid_transition(from, to)) {
        if (bus_) {
            bus_->publish(JobEvent{
                .kind = EventKind::RejectedTransition,
                .job_name = name_,
                .from = from,
                .to = to,
                .ts = std::chrono::steady_clock::now(),
                .reason = "Transition not allowed by FSM"
            });
        }
        return {false, from, to};
    }

    // ---- Idempotent no-op ----
    if (from == to)
        return {true, from, to};

    // ---- CAS loop ----
    while (!state_.compare_exchange_weak(
        from, to,
        std::memory_order_acq_rel,
        std::memory_order_acquire)) {

        if (!is_valid_transition(from, to)) {
            if (bus_) {
                bus_->publish(JobEvent{
                    .kind = EventKind::RejectedTransition,
                    .job_name = name_,
                    .from = from,
                    .to = to,
                    .ts = std::chrono::steady_clock::now(),
                    .reason = "Transition not allowed by FSM"
                });
            }
            return {false, from, to};
        }
    }

    // ---- Successful transition ----
    if (bus_) {
        bus_->publish(JobEvent{
            .kind = EventKind::StateChanged,
            .job_name = name_,
            .from = from,
            .to = to,
            .ts = std::chrono::steady_clock::now(),
            .reason = {}
        });
    }

    return {true, from, to};
}

// ---- Convenience wrappers ----
bool Job::enqueue() noexcept        { return try_transition(JobState::Queued).ok; }
bool Job::schedule() noexcept       { return try_transition(JobState::Scheduled).ok; }
bool Job::start_spooling() noexcept { return try_transition(JobState::Spooling).ok; }
bool Job::start_printing() noexcept { return try_transition(JobState::Printing).ok; }
bool Job::complete() noexcept       { return try_transition(JobState::Completed).ok; }
bool Job::fail() noexcept           { return try_transition(JobState::Failed).ok; }

bool Job::cancel() noexcept {
    JobState s = state();
    if (s == JobState::Canceled) return true;
    if (is_terminal(s)) return false;
    return try_transition(JobState::Canceled).ok;
}

void Job::set_payload(std::string data) {
    std::lock_guard<std::mutex> lk(payload_mu_);
    payload_ = std::move(data);
}

std::string Job::payload_copy() const {
    std::lock_guard<std::mutex> lk(payload_mu_);
    return payload_;
}





} // namespace printpipe

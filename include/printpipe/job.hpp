#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <mutex>
#include <string_view>
namespace printpipe {

class EventBus;

enum class JobState : std::uint8_t {
    Created,
    Queued,
    Scheduled,
    Spooling,
    Printing,
    Completed,
    Canceled,
    Failed
};

struct TransitionResult {
    bool ok;
    JobState from;
    JobState to;
};

class Job {
public:
    explicit Job(std::string name);

    const std::string& name() const noexcept;
    JobState state() const noexcept;

    TransitionResult try_transition(JobState to) noexcept;

   // Spool payload API (thread-safe)
   void set_payload(std::string data);
   std::string payload_copy() const;


    bool enqueue() noexcept;
    bool schedule() noexcept;
    bool start_spooling() noexcept;
    bool start_printing() noexcept;
    bool complete() noexcept;
    bool fail() noexcept;
    bool cancel() noexcept;

    static bool is_terminal(JobState s) noexcept;

    void set_event_bus(std::shared_ptr<EventBus> bus) { bus_ = std::move(bus); }
    

private:
    static bool is_valid_transition(JobState from, JobState to) noexcept;

    std::string name_;
    std::atomic<JobState> state_{JobState::Created};
    std::shared_ptr<EventBus> bus_;

   mutable std::mutex payload_mu_;
   std::string payload_;


};

} // namespace printpipe

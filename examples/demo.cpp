// examples/demo.cpp
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "printpipe/event_bus.hpp"
#include "printpipe/job.hpp"
#include "printpipe/scheduler.hpp"
#include "printpipe/file_backend.hpp"


using printpipe::EventBus;
using printpipe::EventKind;
using printpipe::Job;
using printpipe::JobState;
using printpipe::Scheduler;

static const char* to_string(JobState s) {
    switch (s) {
        case JobState::Created:   return "Created";
        case JobState::Queued:    return "Queued";
        case JobState::Scheduled: return "Scheduled";
        case JobState::Spooling:  return "Spooling";
        case JobState::Printing:  return "Printing";
        case JobState::Completed: return "Completed";
        case JobState::Canceled:  return "Canceled";
        case JobState::Failed:    return "Failed";
    }
    return "?";
}

static const char* to_string(EventKind k) {
    switch (k) {
        case EventKind::StateChanged:         return "StateChanged";
        case EventKind::RejectedTransition:   return "RejectedTransition";
    }
    return "?";
}

int main() {
    auto bus = std::make_shared<EventBus>();

    auto job = std::make_shared<Job>("demo-doc");
    job->set_event_bus(bus);

    Scheduler sched;
    auto backend = std::make_shared<printpipe::FileBackend>("out");
sched.set_backend(backend);

// Provide something to print
job->set_payload("Hello from PrintPipe!\nThis is backend writing to a file.\n");


    sched.start();

    std::cout << "Submitting job: " << job->name()
              << " (initial=" << to_string(job->state()) << ")\n";

    sched.submit(job);

    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    std::cout << "Requesting cancel...\n";
    job->cancel();

    while (!Job::is_terminal(job->state())) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    sched.stop();

    std::cout << "Final job state: " << to_string(job->state()) << "\n\n";

    auto evs = bus->snapshot();
    std::cout << "--- Events (" << evs.size() << ") ---\n";
    for (const auto& ev : evs) {
        std::cout << to_string(ev.kind)
                  << " job=" << ev.job_name
                  << " " << to_string(ev.from) << " -> " << to_string(ev.to);
        if (!ev.reason.empty()) std::cout << " reason=" << ev.reason;
        std::cout << "\n";
    }

    return 0;
}

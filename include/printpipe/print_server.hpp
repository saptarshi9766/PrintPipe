#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <map>
#include <atomic>
#include <filesystem>

#include "printpipe/job.hpp"
#include "printpipe/spooler.hpp"
#include "printpipe/scheduler.hpp"
#include "printpipe/event_bus.hpp"

namespace printpipe {

class PrintServer {
public:
    PrintServer(int port = 8080, std::filesystem::path output_dir = "out");
    ~PrintServer();

    // Start the HTTP server (blocking)
    void start();
    
    // Stop the HTTP server
    void stop();

    // Set the spooler implementation
    void set_spooler(SpoolerPtr spooler);

    // Get event bus for monitoring
    std::shared_ptr<EventBus> event_bus() const { return event_bus_; }

private:
    struct JobEntry {
        std::shared_ptr<Job> job;
        std::filesystem::path output_file;
    };

    int port_;
    std::filesystem::path output_dir_;
    std::shared_ptr<EventBus> event_bus_;
    std::shared_ptr<Scheduler> scheduler_;
    
    std::mutex jobs_mutex_;
    std::map<std::string, JobEntry> jobs_;
    std::atomic<uint64_t> job_counter_{0};

    // Helper methods
    std::string create_job(const std::string& name, const std::string& payload);
    bool submit_job(const std::string& job_id);
    std::string get_job_status(const std::string& job_id);
    std::vector<std::string> list_jobs();
    std::string get_output_file(const std::string& job_id);
};

} // namespace printpipe

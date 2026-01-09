#include "printpipe/print_server.hpp"
#include "printpipe/file_backend.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <csignal>
#include <atomic>

using json = nlohmann::json;

namespace printpipe {

// Global pointer for signal handler
static std::atomic<httplib::Server*> g_server{nullptr};
static std::atomic<bool> g_shutdown_requested{false};

static void signal_handler(int signal) {
    // Prevent recursive signal handling
    if (g_shutdown_requested.exchange(true)) {
        return;
    }
    
    std::cout << "\n[Main] Caught signal " << signal << ", shutting down...\n";
    auto* server = g_server.load();
    if (server) {
        server->stop();
    }
}

static const char* job_state_to_string(JobState s) {
    switch (s) {
        case JobState::Created:   return "created";
        case JobState::Queued:    return "queued";
        case JobState::Scheduled: return "scheduled";
        case JobState::Spooling:  return "spooling";
        case JobState::Printing:  return "printing";
        case JobState::Completed: return "completed";
        case JobState::Canceled:  return "canceled";
        case JobState::Failed:    return "failed";
    }
    return "unknown";
}

PrintServer::PrintServer(int port, std::filesystem::path output_dir)
    : port_(port)
    , output_dir_(std::move(output_dir))
    , event_bus_(std::make_shared<EventBus>())
    , scheduler_(std::make_shared<Scheduler>())
{
    // Set up scheduler with file backend
    auto backend = std::make_shared<FileBackend>(output_dir_);
    scheduler_->set_backend(backend);
    scheduler_->start();
}

PrintServer::~PrintServer() {
    scheduler_->stop();
    stop();
}

void PrintServer::set_spooler(SpoolerPtr spooler) {
    scheduler_->set_spooler(std::move(spooler));
}

std::string PrintServer::create_job(const std::string& name, const std::string& payload) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    
    uint64_t id = job_counter_.fetch_add(1);
    std::ostringstream oss;
    oss << "job-" << std::setfill('0') << std::setw(6) << id;
    std::string job_id = oss.str();
    
    auto job = std::make_shared<Job>(name);
    job->set_event_bus(event_bus_);
    job->set_payload(payload);
    
    auto output_file = output_dir_ / (name + ".txt");
    jobs_[job_id] = JobEntry{job, output_file};
    
    std::cout << "[PrintServer] Created job: " << job_id 
              << " (name: " << name << ", output: " << output_file << ")\n";
    
    return job_id;
}

bool PrintServer::submit_job(const std::string& job_id) {
    std::shared_ptr<Job> job;
    
    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);
        auto it = jobs_.find(job_id);
        if (it == jobs_.end()) {
            return false;
        }
        job = it->second.job;
    }
    
    // Submit to scheduler for async processing
    bool submitted = scheduler_->submit(job);
    
    if (submitted) {
        std::cout << "[PrintServer] Submitted job " << job_id << " to scheduler\n";
    } else {
        std::cout << "[PrintServer] Failed to submit job " << job_id << "\n";
    }
    
    return submitted;
}

std::string PrintServer::get_job_status(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return "{}";
    }
    
    json j;
    j["job_id"] = job_id;
    j["name"] = it->second.job->name();
    j["state"] = job_state_to_string(it->second.job->state());
    j["output_file"] = it->second.output_file.string();
    j["file_exists"] = std::filesystem::exists(it->second.output_file);
    
    return j.dump(2);
}

std::vector<std::string> PrintServer::list_jobs() {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    
    std::vector<std::string> job_ids;
    job_ids.reserve(jobs_.size());
    
    for (const auto& [id, _] : jobs_) {
        job_ids.push_back(id);
    }
    
    return job_ids;
}

std::string PrintServer::get_output_file(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return {};
    }
    
    if (!std::filesystem::exists(it->second.output_file)) {
        return {};
    }
    
    std::ifstream file(it->second.output_file);
    if (!file) {
        return {};
    }
    
    return std::string(std::istreambuf_iterator<char>(file),
                      std::istreambuf_iterator<char>());
}

void PrintServer::start() {
    httplib::Server server;
    
    // Serve web UI
    server.Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream file("web/index.html");
        if (file) {
            std::string html((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
            res.set_content(html, "text/html");
        } else {
            // Fallback to API info
            json j;
            j["service"] = "PrintPipe Virtual Printer";
            j["version"] = "0.1.0";
            j["description"] = "Professional print job management with async processing";
            j["web_ui"] = "Place index.html in web/ directory to enable web interface";
            j["endpoints"] = {
                {"POST /api/jobs", "Create a new print job"},
                {"POST /api/jobs/:id/submit", "Submit a job for async processing"},
                {"GET /api/jobs/:id", "Get job status"},
                {"GET /api/jobs/:id/output", "Download job output file"},
                {"GET /api/jobs", "List all jobs"},
                {"GET /api/events", "Get all events"}
            };
            res.set_content(j.dump(2), "application/json");
        }
    });
    
    // Create a new job
    server.Post("/api/jobs", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string name = body.value("name", "untitled");
            std::string payload = body.value("payload", "Default print content");
            
            std::string job_id = create_job(name, payload);
            
            json response;
            response["job_id"] = job_id;
            response["status"] = "created";
            response["message"] = "Job created. Use /api/jobs/" + job_id + "/submit to process";
            
            res.status = 201;
            res.set_content(response.dump(2), "application/json");
        } catch (const std::exception& e) {
            json error;
            error["error"] = e.what();
            res.status = 400;
            res.set_content(error.dump(2), "application/json");
        }
    });
    
    // Submit a job for processing
    server.Post("/api/jobs/:id/submit", [this](const httplib::Request& req, httplib::Response& res) {
        std::string job_id = req.path_params.at("id");
        
        if (submit_job(job_id)) {
            json response;
            response["job_id"] = job_id;
            response["status"] = "submitted";
            response["message"] = "Job submitted for async processing. Check status at /api/jobs/" + job_id;
            
            res.set_content(response.dump(2), "application/json");
        } else {
            json error;
            error["error"] = "Failed to submit job (not found or already submitted)";
            res.status = 400;
            res.set_content(error.dump(2), "application/json");
        }
    });
    
    // Get job status
    server.Get("/api/jobs/:id", [this](const httplib::Request& req, httplib::Response& res) {
        std::string job_id = req.path_params.at("id");
        std::string status_json = get_job_status(job_id);
        
        if (status_json == "{}") {
            json error;
            error["error"] = "Job not found";
            res.status = 404;
            res.set_content(error.dump(2), "application/json");
        } else {
            res.set_content(status_json, "application/json");
        }
    });
    
    // Download job output
    server.Get("/api/jobs/:id/output", [this](const httplib::Request& req, httplib::Response& res) {
        std::string job_id = req.path_params.at("id");
        std::string output = get_output_file(job_id);
        
        if (output.empty()) {
            json error;
            error["error"] = "Output file not found (job not completed or doesn't exist)";
            res.status = 404;
            res.set_content(error.dump(2), "application/json");
        } else {
            res.set_content(output, "text/plain");
        }
    });
    
    // List all jobs
    server.Get("/api/jobs", [this](const httplib::Request&, httplib::Response& res) {
        auto job_ids = list_jobs();
        
        json j = json::array();
        for (const auto& job_id : job_ids) {
            j.push_back(json::parse(get_job_status(job_id)));
        }
        
        res.set_content(j.dump(2), "application/json");
    });
    
    // Get all events
    server.Get("/api/events", [this](const httplib::Request&, httplib::Response& res) {
        auto events = event_bus_->snapshot();
        
        json j = json::array();
        for (const auto& event : events) {
            json ev;
            ev["kind"] = (event.kind == EventKind::StateChanged) ? "state_changed" : "rejected_transition";
            ev["job_name"] = event.job_name;
            ev["from"] = job_state_to_string(event.from);
            ev["to"] = job_state_to_string(event.to);
            ev["reason"] = event.reason;
            j.push_back(ev);
        }
        
        res.set_content(j.dump(2), "application/json");
    });
    
    std::cout << "[PrintServer] Starting HTTP server on port " << port_ << "...\n";
    std::cout << "[PrintServer] Output directory: " << std::filesystem::absolute(output_dir_) << "\n";
    std::cout << "[PrintServer] Access at http://localhost:" << port_ << "\n";
    
    // Install signal handlers for graceful shutdown
    g_server.store(&server);
    std::signal(SIGINT, signal_handler);   // Ctrl+C
    std::signal(SIGTERM, signal_handler);  // kill command
    
    server.listen("0.0.0.0", port_);
    
    g_server.store(nullptr);
}

void PrintServer::stop() {
    // Cleanup handled by signal handler
}

} // namespace printpipe

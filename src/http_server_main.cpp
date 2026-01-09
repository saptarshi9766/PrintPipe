#include <iostream>
#include <csignal>
#include <atomic>

#include "printpipe/print_server.hpp"

std::atomic<bool> running{true};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[Main] Shutting down...\n";
        running = false;
    }
}

int main(int argc, char* argv[]) {
    int port = 8080;
    
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid port number. Using default: 8080\n";
        }
    }
    
    // Set up signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    std::cout << "PrintPipe HTTP Server\n";
    std::cout << "=====================\n\n";
    
    printpipe::PrintServer server(port);
    
    std::cout << "Starting server on port " << port << "...\n";
    std::cout << "Press Ctrl+C to stop\n\n";
    std::cout << "Example usage:\n";
    std::cout << "  curl -X POST http://localhost:" << port << "/api/jobs -H 'Content-Type: application/json' -d '{\"name\": \"test-job\"}'\n";
    std::cout << "  curl -X POST http://localhost:" << port << "/api/jobs/job-000000/submit\n";
    std::cout << "  curl http://localhost:" << port << "/api/jobs/job-000000\n";
    std::cout << "  curl http://localhost:" << port << "/api/jobs\n\n";
    
    server.start();
    
    return 0;
}

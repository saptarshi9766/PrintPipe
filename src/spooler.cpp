#include "printpipe/spooler.hpp"

#include <sstream>

namespace printpipe {

SpoolResult TextSpooler::spool(const Job& job) {
    std::ostringstream oss;
    oss << "=== PrintPipe Spool ===\n";
    oss << "Job: " << job.name() << "\n";
    oss << "Payload: Hello from PrintPipe!\n";

    const std::string text = oss.str();

    SpoolBuffer buf;
    buf.mime = "text/plain; charset=utf-8";
    buf.bytes.assign(text.begin(), text.end());

    return SpoolResult{true, std::move(buf), {}};
}

} // namespace printpipe

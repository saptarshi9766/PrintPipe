#pragma once

#include <memory>
#include <optional>
#include <string>

#include "printpipe/job.hpp"
#include "printpipe/spool_buffer.hpp"

namespace printpipe {

struct SpoolResult {
    bool ok;
    std::optional<SpoolBuffer> buffer;
    std::string error;
};

class ISpooler {
public:
    virtual ~ISpooler() = default;
    virtual SpoolResult spool(const Job& job) = 0;
};

class TextSpooler final : public ISpooler {
public:
    SpoolResult spool(const Job& job) override;
};

using SpoolerPtr = std::shared_ptr<ISpooler>;

} // namespace printpipe

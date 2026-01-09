#include <catch2/catch_test_macros.hpp>
#include "printpipe/job.hpp"

using namespace printpipe;

TEST_CASE("Job follows happy lifecycle") {
    Job job{"doc"};

    REQUIRE(job.state() == JobState::Created);
    REQUIRE(job.enqueue());
    REQUIRE(job.schedule());
    REQUIRE(job.start_spooling());
    REQUIRE(job.start_printing());
    REQUIRE(job.complete());

    REQUIRE(job.state() == JobState::Completed);
}

TEST_CASE("Cancellation is idempotent") {
    Job job{"doc"};

    REQUIRE(job.enqueue());
    REQUIRE(job.cancel());
    REQUIRE(job.state() == JobState::Canceled);
    REQUIRE(job.cancel());  // idempotent
}

TEST_CASE("Invalid transitions are rejected") {
    Job job{"doc"};

    REQUIRE_FALSE(job.start_printing());
    REQUIRE(job.state() == JobState::Created);
}

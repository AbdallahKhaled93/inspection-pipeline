#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <stop_token>
#include <thread>
#include <vector>

#include "inspection/frame_queue.hpp"

using namespace inspection;
using namespace std::chrono_literals;

namespace {

// Small helper: a frame carrying only an id, which is all these tests need.
AnnotatedFrame make_frame(std::uint64_t id) {
    return AnnotatedFrame{Frame{id, 2, 2}, std::nullopt};
}

// A token that is never stopped, for the tests that expect pop() to succeed.
std::stop_token never_stopped() {
    static std::stop_source source;
    return source.get_token();
}

}  // namespace

TEST_CASE("queue returns frames in order when under capacity") {
    FrameQueue queue{4};

    queue.push(make_frame(10));
    queue.push(make_frame(11));
    queue.push(make_frame(12));

    REQUIRE(queue.pop(never_stopped())->frame.id() == 10);
    REQUIRE(queue.pop(never_stopped())->frame.id() == 11);
    REQUIRE(queue.pop(never_stopped())->frame.id() == 12);
    REQUIRE(queue.dropped_count() == 0);
}

TEST_CASE("queue drops the oldest frame when full, keeping the newest") {
    FrameQueue queue{4};

    // Six frames into a queue of four: ids 0 and 1 should be discarded.
    for (std::uint64_t id = 0; id < 6; ++id) {
        queue.push(make_frame(id));
    }

    std::vector<std::uint64_t> survivors;
    for (int i = 0; i < 4; ++i) {
        survivors.push_back(queue.pop(never_stopped())->frame.id());
    }

    // This is the policy under test: the freshest four frames survive.
    REQUIRE(survivors == std::vector<std::uint64_t>{2, 3, 4, 5});
}

TEST_CASE("dropped_count reports every discarded frame") {
    FrameQueue queue{2};

    for (std::uint64_t id = 0; id < 10; ++id) {
        queue.push(make_frame(id));
    }

    // 10 pushed, 2 retained -> 8 dropped.
    REQUIRE(queue.dropped_count() == 8);
}

TEST_CASE("queue never grows beyond its capacity") {
    FrameQueue queue{3};

    for (std::uint64_t id = 0; id < 100; ++id) {
        queue.push(make_frame(id));
    }

    // Exactly three frames should come out before the queue is empty.
    for (int i = 0; i < 3; ++i) {
        REQUIRE(queue.pop(never_stopped()).has_value());
    }
    REQUIRE(queue.dropped_count() == 97);
}

TEST_CASE("pop returns nullopt when a stop is requested") {
    FrameQueue queue{4};
    std::stop_source source;

    // A consumer blocked on an empty queue must wake when asked to stop,
    // otherwise shutdown deadlocks. This is the test that catches a hang.
    std::optional<AnnotatedFrame> result;
    std::atomic<bool> finished{false};

    std::thread consumer{[&] {
        result = queue.pop(source.get_token());
        finished = true;
    }};

    std::this_thread::sleep_for(50ms);   // let it block in wait()
    REQUIRE_FALSE(finished.load());      // still asleep, as expected

    source.request_stop();
    consumer.join();

    REQUIRE(finished.load());
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("pop returns a frame that arrives while it is blocked") {
    FrameQueue queue{4};
    std::stop_source source;

    std::optional<AnnotatedFrame> result;
    std::thread consumer{[&] { result = queue.pop(source.get_token()); }};

    std::this_thread::sleep_for(50ms);
    queue.push(make_frame(42));
    consumer.join();

    REQUIRE(result.has_value());
    REQUIRE(result->frame.id() == 42);
}

TEST_CASE("no frame is lost or duplicated under concurrent load") {
    FrameQueue queue{8};
    std::stop_source source;

    constexpr std::uint64_t kTotal = 5000;
    std::atomic<std::uint64_t> consumed{0};

    std::thread consumer{[&] {
        while (auto frame = queue.pop(source.get_token())) {
            ++consumed;
        }
    }};

    for (std::uint64_t id = 0; id < kTotal; ++id) {
        queue.push(make_frame(id));
    }

    // Give the consumer a moment to drain, then shut it down.
    std::this_thread::sleep_for(200ms);
    source.request_stop();
    consumer.join();

    // Every frame is accounted for: processed or deliberately dropped.
    // Nothing vanishes silently.
    REQUIRE(consumed.load() + queue.dropped_count() == kTotal);
}
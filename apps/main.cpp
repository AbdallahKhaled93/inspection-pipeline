#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#include "inspection/synthetic_source.hpp"
#include "inspection/frame_queue.hpp"
#include "inspection/producer.hpp"
#include "inspection/analyzer.hpp"


int main() {
    inspection::FrameQueue queue{4};
    inspection::Producer producer{queue, {.width = 64, .height = 32}, {.period = std::chrono::milliseconds{10}}};
    inspection::ThresholdAnalyzer analyzer;

    std::size_t processed = 0;
    std::size_t detected = 0;


    std::jthread consumer{[&](std::stop_token token) {
    while (auto annotated = queue.pop(token)) {
        const auto result = analyzer.analyze(annotated->frame);
        ++processed;
        if (result.detection) { ++detected; }
    }
    }};

    std::this_thread::sleep_for(std::chrono::seconds{2});

    producer.stop();
    consumer.request_stop();
    consumer.join();

    std::cout << "processed:  " << processed << '\n'
              << "with object: " << detected << '\n'
              << "dropped:     " << queue.dropped_count() << '\n';
}
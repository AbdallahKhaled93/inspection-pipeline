#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#include "inspection/synthetic_source.hpp"
#include "inspection/frame_queue.hpp"
#include "inspection/producer.hpp"

namespace {

// Dark -> bright, used to draw the frame in the terminal.
constexpr std::string_view kRamp = " .:-=+*#%@";

// Steps y by 2 because terminal cells are about twice as tall as they are
// wide, which keeps the aspect ratio looking roughly right.
void render(const inspection::Frame& frame) {
    for (std::size_t y = 0; y < frame.height(); y += 2) {
        std::string line;
        line.reserve(frame.width());
        for (std::size_t x = 0; x < frame.width(); ++x) {
            const std::size_t level = frame.at(x, y);
            line.push_back(kRamp[level * (kRamp.size() - 1) / 255]);
        }
        std::cout << "  " << line << '\n';
    }
}

}  // namespace

int main() {
    inspection::FrameQueue queue{4};
    inspection::Producer producer{queue, {}, {}};

    std::size_t processed = 0;
    std::size_t with_object = 0;

    std::jthread consumer{[&](std::stop_token token) {
        while (auto annotated = queue.pop(token)) {
            ++processed;
            if (annotated->ground_truth) { ++with_object; }
        }
    }};

    std::this_thread::sleep_for(std::chrono::seconds{2});

    producer.stop();
    consumer.request_stop();

    std::cout << "processed:  " << processed << '\n'
              << "with object: " << with_object << '\n'
              << "dropped:     " << queue.dropped_count() << '\n';
}
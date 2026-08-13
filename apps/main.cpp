#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#include "inspection/synthetic_source.hpp"

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
    inspection::SyntheticSource source;

    std::cout << "synthetic source -- 8 frames\n\n";

    for (int i = 0; i < 8; ++i) {
        // Structured binding: unpack AnnotatedFrame into two named variables.
        auto&& [frame, ground_truth] = source.next();

        std::cout << "frame #" << frame.id() << "  "
                  << frame.width() << "x" << frame.height() << "  ";

        if (ground_truth) {
            std::cout << "object " << ground_truth->width << "x" << ground_truth->height
                      << " at (" << ground_truth->x << ", " << ground_truth->y << ")\n";
        } else {
            std::cout << "empty\n";
        }

        render(frame);
        std::cout << '\n';
    }

    return 0;
}
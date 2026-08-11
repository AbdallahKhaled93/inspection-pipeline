#include <chrono>
#include <iostream>

#include "inspection/frame.hpp"

int main() {
    inspection::Frame frame{42, 8, 4};

    std::cout << "frame #" << frame.id()
              << "  " << frame.width() << "x" << frame.height()
              << "  pixels=" << frame.pixels().size() << '\n';

    // Non-const at() returns a reference, so the call can sit on the left
    // of an assignment.
    frame.at(3, 1) = 255;

    std::cout << "at(3,1) = " << static_cast<int>(frame.at(3, 1)) << '\n';
    std::cout << "at(0,0) = " << static_cast<int>(frame.at(0, 0)) << '\n';

    // A const reference selects the const overload -> read-only span.
    const inspection::Frame& view = frame;
    std::size_t non_zero = 0;
    for (std::uint8_t pixel : view.pixels()) {
        if (pixel != 0) { ++non_zero; }
    }
    std::cout << "non-zero pixels: " << non_zero << '\n';

    const auto age = inspection::Clock::now() - frame.captured_at();
    std::cout << "frame age: "
              << std::chrono::duration_cast<std::chrono::microseconds>(age).count()
              << " us\n";

    return 0;
}
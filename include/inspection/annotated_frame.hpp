#ifndef INSPECTION_ANNOTATED_FRAME_HPP
#define INSPECTION_ANNOTATED_FRAME_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include "inspection/frame.hpp"

namespace inspection {

    struct Detection {
        std::size_t x{};
        std::size_t y{};
        std::size_t width{};
        std::size_t height{};
    };

    struct AnnotatedFrame {
        Frame frame;
        std::optional<Detection> ground_truth;
    };
} // namespace inspection


#endif // INSPECTION_ANNOTATED_FRAME_HPP
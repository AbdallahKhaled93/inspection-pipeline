#ifndef INSPECTION_FRAME_HPP
#define INSPECTION_FRAME_HPP

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <cassert>

namespace inspection {

    using Clock = std::chrono::steady_clock;

    class Frame {
    public:
        // Constructors
        Frame() = default;
        Frame(std::uint64_t id, std::size_t width, std::size_t height)    
        : id_{id}
        , width_{width}
        , height_{height}
        , captured_at_{Clock::now()}
        , pixels_(width * height, std::uint8_t{0}) {}

        // Getters
        [[nodiscard]] std::uint64_t id() const noexcept { return id_; }
        [[nodiscard]] std::size_t width() const noexcept { return width_; }
        [[nodiscard]] std::size_t height() const noexcept { return height_; }
        [[nodiscard]] Clock::time_point captured_at() const noexcept { return captured_at_; }
        [[nodiscard]] std::span<const std::uint8_t> pixels() const noexcept { return pixels_; }
        [[nodiscard]] std::span<std::uint8_t> pixels() noexcept { return pixels_; }
        [[nodiscard]] std::uint8_t at(std::size_t x, std::size_t y) const noexcept {
            assert(x < width_ && y < height_);
            return pixels_[y * width_ + x];
        }

        [[nodiscard]] std::uint8_t& at(std::size_t x, std::size_t y) noexcept {
            assert(x < width_ && y < height_);
            return pixels_[y * width_ + x];
        }

    private:
        std::uint64_t id_ {0};
        std::size_t width_ {0};
        std::size_t height_ {0};
        Clock::time_point captured_at_ {};
        std::vector<std::uint8_t> pixels_ {};
    };

} // namespace inspection

#endif // INSPECTION_FRAME_HPP
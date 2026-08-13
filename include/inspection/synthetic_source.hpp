#ifndef INSPECTION_SYNTHETIC_SOURCE_HPP
#define INSPECTION_SYNTHETIC_SOURCE_HPP

#include <cstdint>
#include <cstddef>
#include <random>
#include "inspection/annotated_frame.hpp"

namespace inspection
{
    class SyntheticSource
    {
    public:
        struct Config
        {
            std::size_t width{64};
            std::size_t height{32};
            double detection_probability{0.35};
            std::uint8_t background_level{40};
            std::uint8_t object_level{225};
            std::uint8_t noise_amplitude{12};
            std::uint32_t seed{20260811};
        };

        SyntheticSource() : SyntheticSource(Config{}) {}
        explicit SyntheticSource(Config config);

        [[nodiscard]] AnnotatedFrame next();

    private:
        Config config_;
        std::mt19937 rng_;
        std::uint64_t next_id_{0};
    };
} // namespace inspection

#endif // INSPECTION_SYNTHETIC_SOURCE_HPP
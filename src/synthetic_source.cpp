#include "inspection/synthetic_source.hpp"
#include <algorithm>
#include <utility>

namespace inspection
{
    SyntheticSource::SyntheticSource(Config config)
        : config_{config}, rng_{config.seed} {}

    AnnotatedFrame SyntheticSource::next()
    {
        Frame frame(next_id_++, config_.width, config_.height);
        const int amplitude = static_cast<int>(config_.noise_amplitude);
        std::uniform_int_distribution<int> noise{-amplitude, amplitude};

        for (std::uint8_t &pixel : frame.pixels())
        {
            const int value = static_cast<int>(config_.background_level) + noise(rng_);
            pixel = static_cast<std::uint8_t>(std::clamp(value, 0, 255));
        }

        std::optional<Detection> ground_truth;
        std::bernoulli_distribution should_inject{config_.detection_probability};
        if (should_inject(rng_))
        {
            std::uniform_int_distribution<std::size_t> size_dist{3, 7};
            const std::size_t box_width = size_dist(rng_);
            const std::size_t box_height = size_dist(rng_);
            std::uniform_int_distribution<std::size_t> x_dist{0, config_.width - box_width};
            std::uniform_int_distribution<std::size_t> y_dist{0, config_.height - box_height};
            ground_truth = Detection{
                .x = x_dist(rng_),
                .y = y_dist(rng_),
                .width = box_width,
                .height = box_height,
            };

            for (std::size_t y = ground_truth->y; y < ground_truth->y + ground_truth->height; ++y)
            {
                for (std::size_t x = ground_truth->x; x < ground_truth->x + ground_truth->width; ++x)
                {
                    frame.at(x, y) = config_.object_level;
                }
            }
        }

        return AnnotatedFrame{std::move(frame), ground_truth};
    }
}
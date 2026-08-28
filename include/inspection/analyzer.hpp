#ifndef INSPECTION_ANALYZER_HPP
#define INSPECTION_ANALYZER_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <chrono>
#include <algorithm>
#include "inspection/annotated_frame.hpp"

namespace inspection
{
    struct AnalysisResult
    {
        std::uint64_t frame_id{};
        std::optional<Detection> detection;
        Clock::duration processing_time{};
    };

    class Analyzer
    {
    public:
        virtual ~Analyzer() = default;
        [[nodiscard]] virtual AnalysisResult analyze(const Frame &frame) = 0;

        Analyzer(const Analyzer &) = delete;
        Analyzer &operator=(const Analyzer &) = delete;

    protected:
        Analyzer() = default;
    };

    class ThresholdAnalyzer final : public Analyzer
    {
    public:
        struct Config
        {
            std::uint8_t threshold{128};
            std::size_t min_pixels{4};
        };

        ThresholdAnalyzer() : ThresholdAnalyzer(Config{}) {}
        explicit ThresholdAnalyzer(Config config) : config_{config} {}

        [[nodiscard]] AnalysisResult analyze(const Frame &frame) override
        {
            const auto start_time = Clock::now();
            std::size_t min_x = frame.width();
            std::size_t max_x = 0;
            std::size_t min_y = frame.height();
            std::size_t max_y = 0;
            std::size_t bright_pixels = 0;

            for (std::size_t y = 0; y < frame.height(); ++y)
            {
                for (std::size_t x = 0; x < frame.width(); ++x)
                {
                    if (frame.at(x, y) >= config_.threshold)
                    {
                        ++bright_pixels;
                        min_x = std::min(min_x, x);
                        max_x = std::max(max_x, x);
                        min_y = std::min(min_y, y);
                        max_y = std::max(max_y, y);
                    }
                }
            }

            std::optional<Detection> detection;
            if (bright_pixels >= config_.min_pixels)
            {
                detection = Detection{
                    .x = min_x,
                    .y = min_y,
                    .width = max_x - min_x + 1,
                    .height = max_y - min_y + 1};
            }

            return AnalysisResult{
                .frame_id = frame.id(),
                .detection = detection,
                .processing_time = Clock::now() - start_time};
        }

    private:
        Config config_;
    };

    /// Does nothing. Used to measure the pipeline's own cost without analysis.
    class NullAnalyzer final : public Analyzer
    {
    public:
        [[nodiscard]] AnalysisResult analyze(const Frame &frame) override
        {
            return AnalysisResult{.frame_id = frame.id(), .detection = std::nullopt, .processing_time = {}};
        }
    };
} // namespace inspection

#endif // INSPECTION_ANALYZER_HPP
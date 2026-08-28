#ifndef INSPECTION_FRAME_QUEUE_HPP
#define INSPECTION_FRAME_QUEUE_HPP

#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <condition_variable>
#include <stop_token>
#include "inspection/annotated_frame.hpp"

namespace inspection {
    class FrameQueue 
    {
        public:
            explicit FrameQueue(std::size_t capacity);
            void push(AnnotatedFrame frame);
            [[nodiscard]] std::optional<AnnotatedFrame> pop(std::stop_token token);
            [[nodiscard]] std::size_t dropped_count() const;
        private:
            const std::size_t capacity_;
            std::deque<AnnotatedFrame> items_;
            mutable std::mutex mutex_;
            std::condition_variable_any not_empty_;
            std::size_t dropped_count_{0};
    };
} // namespace inspection


#endif // INSPECTION_FRAME_QUEUE_HPP
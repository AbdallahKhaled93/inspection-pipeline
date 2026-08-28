#include "inspection/frame_queue.hpp"

namespace inspection
{
    FrameQueue::FrameQueue(std::size_t capacity)
        : capacity_{capacity} {}

    void FrameQueue::push(AnnotatedFrame frame) {
        { // Start of critical section
            std::lock_guard lock{mutex_};

            if (items_.size() >= capacity_) {
                items_.pop_front();      // drop the oldest
                ++dropped_count_;
            }
            items_.push_back(std::move(frame));
        } // End of critical section
        not_empty_.notify_one();
    }

    std::optional<AnnotatedFrame> FrameQueue::pop(std::stop_token token)
    {
        std::unique_lock lock{mutex_};

        if(!not_empty_.wait(lock, token, [this] { return !items_.empty(); })) {
            return std::nullopt; // Stop requested
        }

        AnnotatedFrame frame = std::move(items_.front());
        items_.pop_front();
        return frame;
    }

    std::size_t FrameQueue::dropped_count() const 
    {
        std::lock_guard lock{mutex_};
        return dropped_count_;
    }
}  // namespace inspection
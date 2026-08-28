#include "inspection/producer.hpp"

namespace inspection
{
    Producer::Producer(FrameQueue& queue, SyntheticSource::Config source_config, Config config)
        : queue_{queue}, source_{source_config}, config_{config}, thread_{[this](std::stop_token token) { run(token);}}
    {

    }

    Producer::~Producer() = default;

    void Producer::run(std::stop_token token) 
    {
        auto next = Clock::now();

        while (!token.stop_requested()) 
        {
            queue_.push(source_.next());

            next += config_.period;

            // handle overrun: if we're already past the deadline, skip missed periods
            const auto now = Clock::now();
            if (next < now) {
                next = now + config_.period;
            }

            std::this_thread::sleep_until(next);
        }
    }

    void Producer::stop()
    {
        thread_.request_stop();
    }
} // namespace inspection
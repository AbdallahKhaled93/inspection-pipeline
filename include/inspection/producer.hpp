#ifndef INSPECTION_PRODUCER_HPP
#define INSPECTION_PRODUCER_HPP

#include <chrono>
#include <thread>
#include <stop_token>
#include "inspection/synthetic_source.hpp"
#include "inspection/frame_queue.hpp"

namespace inspection {
    class Producer 
    {
        public:
            struct Config {
              std::chrono::milliseconds period{33}; // 30 FPS  
            };

            Producer(FrameQueue& queue, SyntheticSource::Config source_config, Config config);
            ~Producer();

            // Delete copy constructors and assignment operators
            Producer(const Producer&) = delete;
            Producer& operator=(const Producer&) = delete;

            void stop();

        private:
            void run(std::stop_token token);
            FrameQueue& queue_;
            SyntheticSource source_;
            Config config_;
            std::jthread thread_;
    };
} // namespace inspection


#endif // INSPECTION_PRODUCER_HPP
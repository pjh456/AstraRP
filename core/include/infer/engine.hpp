#ifndef INCLUDE_ASTRA_RP_ENGINE_HPP
#define INCLUDE_ASTRA_RP_ENGINE_HPP

#include <mutex>
#include <condition_variable>
#include <thread>
#include <utility>

#include "utils/types.hpp"
#include "core/context.hpp"
#include "core/batch.hpp"
#include "infer/session.hpp"

namespace astra_rp
{
    namespace infer
    {
        class Task;

        class Engine
        {
        private:
            Queue<MulPtr<Task>> m_pending_queue;
            std::mutex m_mtx;
            std::condition_variable m_cv;

            bool m_running = false;
            std::thread m_worker;

        public:
            static Engine &instance();

        private:
            Engine() = default;

        public:
            ~Engine();

            Engine(const Engine &) = delete;
            Engine &operator=(const Engine &) = delete;

            Engine(Engine &&) noexcept = default;
            Engine &operator=(Engine &&) noexcept = default;

        public:
            void start();
            void stop();
            void submit(MulPtr<Task> task);

        private:
            void loop();

            ResultV<void> decode(
                MulPtr<astra_rp::core::Context> ctx,
                MulPtr<astra_rp::core::Batch> batch);

            ResultV<std::pair<MulPtr<core::Batch>, size_t>>
            tok2batch(
                MulPtr<infer::Session> session,
                MulPtr<core::Context> ctx,
                Vec<Token> tokens);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_ENGINE_HPP
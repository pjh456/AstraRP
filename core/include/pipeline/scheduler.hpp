#ifndef INCLUDE_ASTRA_RP_SCHEDULER_HPP
#define INCLUDE_ASTRA_RP_SCHEDULER_HPP

#include <mutex>
#include <condition_variable>
#include <thread>
#include <optional>

#include "utils/types.hpp"
#include "pipeline/event_bus.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        class Graph;

        class Scheduler
        {
        private:
            MulPtr<Graph> m_graph;

            HashMap<Str, int> m_current_in_degrees;
            Queue<Str> m_ready_queue;

            std::mutex m_mtx;
            std::condition_variable m_cv;

            int m_active_tasks = 0;
            bool m_stop = false;

            Vec<std::thread> m_workers;
            int m_max_concurrency;

            std::optional<utils::Error> m_fatal_error = std::nullopt;
            MulPtr<EventBus> m_bus;

        public:
            Scheduler(
                MulPtr<Graph> graph,
                int max_concurrency = 1,
                MulPtr<EventBus> bus = nullptr)
                : m_graph(graph),
                  m_max_concurrency(max_concurrency),
                  m_bus(bus) {}

            ~Scheduler();

            ResultV<void> run();

        private:
            ResultV<void> worker_thread();
        };
    }
}

#endif // INCLUDE_ASTRA_RP_SCHEDULER_HPP
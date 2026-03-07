#ifndef INCLUDE_ASTRA_RP_SCHEDULER_HPP
#define INCLUDE_ASTRA_RP_SCHEDULER_HPP

#include <mutex>
#include <condition_variable>

#include "utils/types.hpp"

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

            std::mutex m_mtx;
            std::condition_variable m_cv;
            int m_active_tasks = 0;

        public:
            Scheduler(MulPtr<Graph> graph)
                : m_graph(graph) {}

            void run();
        };
    }
}

#endif // INCLUDE_ASTRA_RP_SCHEDULER_HPP
#include "pipeline/scheduler.hpp"

#include <thread>
#include <future>

#include "pipeline/graph.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        void Scheduler::run()
        {
            if (!m_graph->validate())
                throw std::runtime_error("Graph has cycles!");

            m_current_in_degrees = m_graph->in_degrees();
            Queue<Str> ready_queue;

            // 1. 初始化：找到所有入度为 0 的节点（即没有前置依赖的起点）
            for (const auto &pair : m_current_in_degrees)
            {
                if (pair.second == 0)
                    ready_queue.push(pair.first);
            }

            // 2. 主调度循环
            while (true)
            {
                Str current_node_id;
                {
                    std::unique_lock<std::mutex> lock(m_mtx);

                    // 等待条件：队列有任务可以执行，或者所有任务都结束了
                    m_cv.wait(
                        lock,
                        [this, &ready_queue]
                        { return !ready_queue.empty() ||
                                 m_active_tasks == 0; });

                    // 如果队列为空且没有活动任务，说明全图执行完毕
                    if (ready_queue.empty() && m_active_tasks == 0)
                        break;

                    current_node_id = ready_queue.front();
                    ready_queue.pop();
                    m_active_tasks++;
                }

                // 3. 异步/线程池中执行该节点
                // 注意：如果 llama.cpp 后端是单卡的 CUDA，你可能不希望真正多线程并发 execute()
                // 而是希望串行压榨。如果是 CPU 或多卡环境，可以使用 std::async 并发
                std::thread(
                    [this, current_node_id, &ready_queue]()
                    {
                        // 1. 获取节点并执行（耗时操作，不加锁，让它们并发！）
                        auto node = m_graph->nodes().at(current_node_id);
                        bool success = node->execute();

                        // 2. 执行完毕后，进入临界区更新图状态和调度队列
                        {
                            std::lock_guard<std::mutex> lock(m_mtx);
                            m_active_tasks--; // 减少活跃任务数

                            if (success)
                            {
                                // 安全查找当前节点的所有下游
                                auto it = m_graph->link_table().find(current_node_id);
                                if (it != m_graph->link_table().end())
                                {
                                    for (const Str &child_id : it->second)
                                    {
                                        // 传递数据给下游
                                        auto child_node = m_graph->nodes().at(child_id);
                                        child_node->set_input(current_node_id, node->output());

                                        // 下游节点入度减 1，为 0 时进入就绪队列
                                        m_current_in_degrees[child_id]--;
                                        if (m_current_in_degrees[child_id] == 0)
                                            ready_queue.push(child_id);
                                    }
                                }
                            }
                        } // 提前释放锁

                        // 3. 唤醒主线程的调度器
                        // 必须在释放锁之后或释放锁前夕调用，防止主线程被唤醒后又被锁卡住
                        m_cv.notify_all();
                    })
                    .detach();
            }
        }
    }
}
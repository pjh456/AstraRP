#include "pipeline/scheduler.hpp"

#include <thread>
#include <future>

#include "pipeline/graph.hpp"

#include "utils/logger.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        Scheduler::~Scheduler()
        {
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                m_stop = true;
            }
            m_cv.notify_all();
            for (auto &t : m_workers)
            {
                if (t.joinable())
                    t.join();
            }
        }

        ResultV<void>
        Scheduler::run()
        {
            auto validation_res = m_graph->validate();
            if (validation_res.is_err())
                return ResultV<void>::Err(
                    validation_res.unwrap_err());

            m_current_in_degrees = m_graph->in_degrees();
            m_stop = false;

            // 1. 初始化入度为 0 的起点
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                for (const auto &pair : m_current_in_degrees)
                {
                    if (pair.second == 0)
                        m_ready_queue.push(pair.first);
                }
            }

            // 2. 启动固定数量的 Worker 线程池
            for (int i = 0; i < m_max_concurrency; ++i)
            {
                m_workers.emplace_back(&Scheduler::worker_thread, this);
            }

            // 3. 主线程阻塞，等待所有 Worker 完成 DAG 执行
            for (auto &t : m_workers)
            {
                if (t.joinable())
                    t.join();
            }

            m_workers.clear();

            if (m_fatal_error.has_value())
                return ResultV<void>::Err(m_fatal_error.value());

            return ResultV<void>::Ok();
        }

        ResultV<void>
        Scheduler::worker_thread()
        {
            while (true)
            {
                Str current_node_id;
                {
                    std::unique_lock<std::mutex> lock(m_mtx);

                    // 等待队列有任务，或者调度器停止
                    m_cv.wait(
                        lock,
                        [this]
                        { return !m_ready_queue.empty() ||
                                 m_stop ||
                                 (m_ready_queue.empty() &&
                                  m_active_tasks == 0); });

                    // 退出条件：收到停止信号，或者队列空且无活跃任务（图执行完毕）
                    if (
                        (m_ready_queue.empty() &&
                         m_active_tasks == 0) ||
                        m_stop)
                        return ResultV<void>::Ok();

                    current_node_id = m_ready_queue.front();
                    m_ready_queue.pop();
                    m_active_tasks++;
                }

                // 1. 离开临界区，执行耗时推理（完美并发）
                auto node = m_graph->nodes().at(current_node_id);

                auto exec_res = node->execute();
                if (exec_res.is_err())
                {
                    auto err = exec_res.unwrap_err();
                    ASTRA_LOG_ERROR("Node " + current_node_id + " execution failed: " + err.to_string());

                    std::lock_guard<std::mutex> lock(m_mtx);
                    if (!m_fatal_error.has_value())
                    {
                        // 保证只记录第一个导致崩溃的错误
                        m_fatal_error = err;
                        m_stop = true; // 【核心】熔断标志：不再接受新任务

                        // 使用 EventBus 发布全局调度器错误
                        if (m_bus)
                            m_bus->publish_error(
                                "SCHEDULER",
                                utils::ErrorBuilder()
                                    .infer()
                                    .message("Pipeline halted due to node failure: " + current_node_id)
                                    .wrap(std::move(err))
                                    .build());
                    }
                    m_cv.notify_all();          // 唤醒所有等待的 worker 让它们安全退出
                    return ResultV<void>::Ok(); // 线程安全退出
                }

                // 2. 重新加锁更新图状态
                {
                    std::lock_guard<std::mutex> lock(m_mtx);
                    m_active_tasks--;

                    if (exec_res.is_ok())
                    {
                        auto it = m_graph->link_table().find(current_node_id);
                        if (it != m_graph->link_table().end())
                        {
                            for (const Str &child_id : it->second)
                            {
                                auto child_node = m_graph->nodes().at(child_id);
                                child_node->set_input(current_node_id, node->output());

                                m_current_in_degrees[child_id]--;
                                if (m_current_in_degrees[child_id] == 0)
                                {
                                    m_ready_queue.push(child_id);
                                }
                            }
                        }
                    }
                } // 释放锁

                // 唤醒可能在等待的其他 worker
                m_cv.notify_all();
            }

            return ResultV<void>::Ok();
        }
    }
}
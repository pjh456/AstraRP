#include "infer/engine.hpp"

#include "llama.h"

#include "core/model.hpp"
#include "core/batch_manager.hpp"
#include "infer/task.hpp"
#include "utils/logger.hpp"

namespace astra_rp
{
    namespace infer
    {
        Engine &Engine::instance()
        {
            static Engine manager;
            return manager;
        }

        Engine::~Engine()
        {
            stop();
        }

        void Engine::start()
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            if (!m_worker.joinable())
            {
                m_running = true;
                m_worker = std::thread(&Engine::loop, this);
                ASTRA_LOG_INFO("Inference Engine background thread started.");
            }
        }

        void Engine::stop()
        {
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                m_running = false;
            }

            m_cv.notify_all();

            if (m_worker.joinable())
            {
                m_worker.join();
                ASTRA_LOG_INFO("Inference Engine background thread stopped.");
            }
        }

        void Engine::submit(MulPtr<Task> task)
        {
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                task->state = TaskState::PENDING;
                m_pending_queue.push(task);
            }
            m_cv.notify_one(); // 唤醒后台线程
        }

        void Engine::loop()
        {
            // active_tasks 维护当前正在交替执行的任务列表
            List<MulPtr<Task>> active_tasks;

            while (true)
            {
                // 1. 获取新任务 / 等待唤醒
                {
                    std::unique_lock<std::mutex> lock(m_mtx);
                    m_cv.wait(
                        lock,
                        [this, &active_tasks]
                        { return (!m_running) ||
                                 (!m_pending_queue.empty()) ||
                                 (!active_tasks.empty()); });

                    if ((!m_running) &&
                        m_pending_queue.empty() &&
                        active_tasks.empty())
                        break;

                    // 将新任务转移到活跃队列
                    while (!m_pending_queue.empty())
                    {
                        active_tasks.push_back(m_pending_queue.front());
                        m_pending_queue.pop();
                    }
                }

                if (active_tasks.empty())
                    continue;

                // 2. 轮转调度：取出队首任务进行处理
                auto task = active_tasks.front();
                active_tasks.pop_front();
                task->state = TaskState::RUNNING;

                auto session = task->session;
                auto ctx = session->context();

                // 3. 动态确定本次 Batch 大小 (由 Context 配置和剩余 Token 数共同决定)
                uint32_t n_batch_max = llama_n_batch(ctx->raw());
                size_t tokens_to_process = std::min((size_t)n_batch_max, task->pending_tokens.size());

                if (tokens_to_process == 0)
                {
                    // 没有需要处理的 Token，可能状态异常，直接结束
                    task->state = TaskState::FINISHED;
                    if (task->on_finish)
                        task->on_finish();
                    task->completion_signal.set_value();
                    continue;
                }

                // 4. 从你的 BatchManager 借用对应大小的 Batch
                auto batch_res = core::BatchManager::instance().acquire(tokens_to_process, 1);
                if (batch_res.is_err())
                {
                    task->state = TaskState::FAILED;
                    if (task->on_error)
                        task->on_error(batch_res.unwrap_err());
                    task->completion_signal.set_value(); // 解除阻塞
                    continue;
                }
                auto batch = batch_res.unwrap();

                // 5. 组装物理 Batch 数据
                bool add_failed = false;
                for (size_t i = 0; i < tokens_to_process; ++i)
                {
                    // 只有这批 pending_tokens 的*最后一个*才需要计算 Logits 用于下一次采样
                    bool is_absolute_last = (i == tokens_to_process - 1) &&
                                            (tokens_to_process == task->pending_tokens.size());

                    auto add_res = batch->add(
                        task->pending_tokens[i],
                        session->n_past() + i,
                        {session->seq_id()},
                        is_absolute_last);

                    if (add_res.is_err())
                    {
                        task->state = TaskState::FAILED;
                        if (task->on_error)
                            task->on_error(add_res.unwrap_err());
                        task->completion_signal.set_value();
                        add_failed = true;
                        break;
                    }
                }

                if (add_failed)
                    continue; // 放弃此任务

                // 6. 核心动作：执行单次物理推理
                auto dec_res = decode(ctx, batch);
                if (dec_res.is_err())
                {
                    task->state = TaskState::FAILED;
                    if (task->on_error)
                        task->on_error(dec_res.unwrap_err());
                    task->completion_signal.set_value();
                    continue;
                }

                // 推进 Session 的时间线
                session->advance_past(tokens_to_process);

                // 从 pending 队列中抹除已经处理完的 tokens
                task->pending_tokens.erase(
                    task->pending_tokens.begin(),
                    task->pending_tokens.begin() + tokens_to_process);

                // 7. 判断当前处于 Prefill(提示词阶段) 还是 Decode(生成阶段)
                if (!task->pending_tokens.empty())
                {
                    // 说明长 Prompt 还没消化完，重新排到队尾，下一轮继续消化
                    active_tasks.push_back(task);
                    continue;
                }

                // --- 此时 pending_tokens 为空，说明我们刚算完需要预测的 Logits ---

                // 8. 采样出下一个 Token
                Token next_token = session->sampler().sample(*ctx, -1);
                session->add_history(next_token); // 归档

                // 检查是否遇到结束符 EOS
                auto vocab = llama_model_get_vocab(session->model()->raw());
                if (llama_vocab_is_eog(vocab, next_token))
                {
                    task->state = TaskState::FINISHED;
                    if (task->on_finish)
                        task->on_finish();
                    task->completion_signal.set_value(); // 任务圆满完成
                    continue;
                }

                // 9. 触发应用层回调
                task->generated_count++;
                bool continue_gen = true;
                if (task->on_token)
                {
                    // 如果外部决定停止 (例如遇到了特定的停用词)，返回 false
                    continue_gen = task->on_token(next_token);
                }

                // 10. 检查生成限制与结束条件
                if (!continue_gen || (task->max_tokens > 0 && task->generated_count >= task->max_tokens))
                {
                    task->state = TaskState::FINISHED;
                    if (task->on_finish)
                        task->on_finish();
                    task->completion_signal.set_value();
                    continue;
                }

                // 11. 将新生成的 Token 放入 pending，重新排到队尾，进入下一轮自回归
                task->pending_tokens.push_back(next_token);
                active_tasks.push_back(task);
            }
        }

        ResultV<void>
        Engine::decode(
            MulPtr<astra_rp::core::Context> ctx,
            MulPtr<astra_rp::core::Batch> batch)
        {
            auto res = llama_decode(ctx->raw(), batch->raw());
            if (res != 0)
            {
                if (res == 1)
                {
                    ASTRA_LOG_ERROR(
                        "Engine decode failed: Batch capacity exceeded. Context model: " +
                        ctx->model().name());
                    return ResultV<void>::Err(
                        utils::ErrorBuilder()
                            .infer()
                            .engine_decode_failed()
                            .message("Batch capacity exceeded. Consider increasing the batch size or reducing the number of tokens.")
                            .context_id(ctx->model().name())
                            .build());
                }
                else if (res == 2)
                {
                    ASTRA_LOG_ERROR(
                        "Engine decode failed: Context state invalid. Context model: " +
                        ctx->model().name());
                    return ResultV<void>::Err(
                        utils::ErrorBuilder()
                            .infer()
                            .engine_decode_failed()
                            .message("Context state invalid. This may be caused by exceeding the context capacity or corrupted state. Consider clearing the context or reducing the number of tokens.")
                            .context_id(ctx->model().name())
                            .build());
                }
                else
                {
                    ASTRA_LOG_ERROR(
                        "Engine decode failed with error code: " +
                        std::to_string(res) +
                        ". Context model: " +
                        ctx->model().name());
                    return ResultV<void>::Err(
                        utils::ErrorBuilder()
                            .infer()
                            .engine_decode_failed()
                            .message("Engine decode failed with error code: " + std::to_string(res))
                            .context_id(ctx->model().name())
                            .build());
                }
            }
            return ResultV<void>::Ok();
        }
    }
}
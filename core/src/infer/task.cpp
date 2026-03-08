#include "infer/task.hpp"

#include "core/tokenizer.hpp"
#include "utils/logger.hpp"

namespace astra_rp
{
    namespace infer
    {
        void Task::wait()
        {
            completion_signal.get_future().wait();
        }

        TaskBuilder::TaskBuilder(MulPtr<Session> session)
            : m_session(std::move(session)) {}

        ResultV<MulPtr<Task>> TaskBuilder::build()
        {
            using TaskRes = ResultV<MulPtr<Task>>;

            if (!m_session)
            {
                return TaskRes::Err(
                    utils::ErrorBuilder()
                        .infer()
                        .invalid_argument()
                        .message("TaskBuilder requires a valid Session.")
                        .build());
            }

            // 1. 创建 Task 实例
            auto task = std::make_shared<Task>(m_session);

            // 2. 处理 Tokenize
            if (m_has_prompt && !m_prompt.empty())
            {
                // TODO: 根据需要决定是否 add_special (通常第一轮对话需要 BOS)
                bool is_first_run = (m_session->n_past() == 0);
                auto tokenize_res = core::Tokenizer::tokenize(
                    m_session->model(),
                    m_prompt,
                    is_first_run,
                    true);

                if (tokenize_res.is_err())
                {
                    return TaskRes::Err(
                        utils::ErrorBuilder()
                            .infer()
                            .tokenize_failed()
                            .wrap(tokenize_res.unwrap_err())
                            .build());
                }
                ASSIGN_OR_RETURN(
                    tokens,
                    core::Tokenizer::tokenize(
                        m_session->model(),
                        m_prompt,
                        is_first_run,
                        true)
                        .map_err(
                            [](auto err)
                            {
                                return utils::Failure(
                                    utils::ErrorBuilder()
                                        .infer()
                                        .tokenize_failed()
                                        .wrap(err)
                                        .build());
                            }));
                task->pending_tokens = std::move(tokens);
            }

            // 3. 动态推导与裁剪 max_tokens
            int32_t capacity = m_session->context()->capacity();
            int32_t current_usage = m_session->n_past();
            int32_t prompt_size = task->pending_tokens.size();

            int32_t available_space = capacity - current_usage - prompt_size;

            if (available_space <= 0)
            {
                return ResultV<MulPtr<Task>>::Err(
                    utils::ErrorBuilder()
                        .infer()
                        .session_state_invalid()
                        .message(
                            "Context capacity exceeded. Capacity: " +
                            std::to_string(capacity) +
                            ", Used: " + std::to_string(current_usage) +
                            ", Prompt: " + std::to_string(prompt_size))
                        .build());
            }

            if (m_max_tokens <= 0)
            {
                // 如果未手动设置，默认榨干剩余可用空间
                // TODO: 设定一个合理的默认上限
                task->max_tokens = available_space;
                ASTRA_LOG_DEBUG(
                    "max_tokens auto-adapted to remaining context size: " +
                    std::to_string(available_space));
            }
            else if (m_max_tokens > available_space)
            {
                // 如果用户手动设置的值溢出了上下文窗口，执行强制安全裁剪
                task->max_tokens = available_space;
                ASTRA_LOG_WARN(
                    "Configured max_tokens (" +
                    std::to_string(m_max_tokens) +
                    ") exceeds available context. Capped to: " +
                    std::to_string(available_space));
            }
            else
            {
                task->max_tokens = m_max_tokens;
            }

            // 4. 包装回调
            auto model_ptr = m_session->model();
            auto user_on_text = m_on_text;

            task->on_token = [model_ptr, user_on_text](Token t) -> bool
            {
                if (!user_on_text)
                    return true; // 如果没注册回调，默认继续生成

                auto str_res = core::Tokenizer::detokenize(model_ptr, {t}, false, false);
                if (str_res.is_err())
                {
                    ASTRA_LOG_ERROR("Detokenize failed in Engine callback.");
                    return false; // 解码失败，强行中止
                }

                return user_on_text(str_res.unwrap());
            };

            task->on_finish = m_on_finish;
            task->on_error = m_on_error;

            return ResultV<MulPtr<Task>>::Ok(std::move(task));
        }

        TaskBuilder &TaskBuilder::prompt(const Str &text)
        {
            m_prompt = text;
            m_has_prompt = true;
            return *this;
        }

        TaskBuilder &TaskBuilder::max_tokens(int32_t count)
        {
            m_max_tokens = count;
            return *this;
        }

        TaskBuilder &TaskBuilder::on_text_generated(std::function<bool(const Str &)> cb)
        {
            m_on_text = std::move(cb);
            return *this;
        }

        TaskBuilder &TaskBuilder::on_finish(std::function<void()> cb)
        {
            m_on_finish = std::move(cb);
            return *this;
        }

        TaskBuilder &TaskBuilder::on_error(std::function<void(const utils::Error &)> cb)
        {
            m_on_error = std::move(cb);
            return *this;
        }
    }
}
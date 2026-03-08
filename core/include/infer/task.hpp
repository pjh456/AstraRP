#ifndef INCLUDE_ASTRA_RP_TASK_HPP
#define INCLUDE_ASTRA_RP_TASK_HPP

#include <functional>
#include <future>

#include "utils/types.hpp"
#include "infer/session.hpp"

namespace astra_rp
{
    namespace infer
    {
        enum class TaskState
        {
            PENDING,
            RUNNING,
            FINISHED,
            FAILED
        };

        struct Task
        {
            MulPtr<Session> session;

            Vec<Token> pending_tokens; // 待处理的 Token (起初是 Prompt，后续是新生成的 Token)
            int32_t max_tokens = -1;   // 最大生成长度 (-1 为无限制)
            int32_t generated_count = 0;
            TaskState state = TaskState::PENDING;

            std::function<bool(Token)> on_token;
            std::function<void()> on_finish;
            std::function<void(const utils::Error &)> on_error;

            std::promise<void> completion_signal;
            Task(MulPtr<Session> s) : session(std::move(s)) {}
        };
    }
}

#endif // INCLUDE_ASTRA_RP_TASK_HPP
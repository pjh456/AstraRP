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

        class TaskBuilder;
        class Engine;

        class Task
        {
            friend class TaskBuilder;
            friend class Engine;

        private:
            MulPtr<Session> session;

            Vec<Token> pending_tokens; // 待处理的 Token (起初是 Prompt，后续是新生成的 Token)
            int32_t max_tokens = -1;   // 最大生成长度 (-1 为无限制)
            int32_t generated_count = 0;
            TaskState m_state = TaskState::PENDING;

            std::function<bool(Token)> on_token;
            std::function<void()> on_finish;
            std::function<void(const utils::Error &)> on_error;

            std::promise<void> completion_signal;

        public:
            Task(MulPtr<Session> s) : session(std::move(s)) {}

            void wait();

        public:
            TaskState state() const noexcept { return m_state; }
        };

        class TaskBuilder
        {
        private:
            MulPtr<Session> m_session;

            // 原始输入缓存
            Str m_prompt;
            bool m_has_prompt = false;

            // 配置参数
            int32_t m_max_tokens = -1; // -1 表示未设置，将动态推导

            // 字符串级别的回调（对外暴露）
            std::function<bool(const Str &)> m_on_text;
            std::function<void()> m_on_finish;
            std::function<void(const utils::Error &)> m_on_error;

        public:
            explicit TaskBuilder(MulPtr<Session> session);

            ResultV<MulPtr<Task>> build();

        public:
            // 设置输入提示词 (内部将自动处理 Tokenize)
            TaskBuilder &prompt(const Str &text);

            // 设置最大生成数量 (若不设置，将根据剩余 Context 自动适配)
            TaskBuilder &max_tokens(int32_t count);

            // 注册文本回调
            TaskBuilder &on_text_generated(std::function<bool(const Str &)> cb);
            TaskBuilder &on_finish(std::function<void()> cb);
            TaskBuilder &on_error(std::function<void(const utils::Error &)> cb);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_TASK_HPP
#ifndef INCLUDE_ASTRA_RP_EVENT_BUS_HPP
#define INCLUDE_ASTRA_RP_EVENT_BUS_HPP

#include <mutex>
#include <functional>

#include "utils/types.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        enum class NodeState
        {
            PENDING,  // 等待依赖完成
            READY,    // 依赖已完成，等待调度
            RUNNING,  // 正在生成
            FINISHED, // 执行成功
            FAILED    // 执行失败
        };

        class EventBus
        {
        public:
            using TokenCallback = std::function<void(const Str &id, const Str &text)>;
            using StateCallback = std::function<void(const Str &id, NodeState state)>;
            using ErrorCallback = std::function<void(const Str &id, utils::Error err)>;

        private:
            Vec<TokenCallback> m_token_subs;
            Vec<StateCallback> m_state_subs;
            Vec<ErrorCallback> m_error_subs;

            std::mutex m_mtx;

        public:
            EventBus() = default;
            ~EventBus() = default;

        public:
            void subscribe_token(TokenCallback cb);
            void subscribe_state(StateCallback cb);
            void subscribe_error(ErrorCallback cb);

        public:
            void publish_token(const Str &id, const Str &text);
            void publish_state(const Str &id, NodeState state);
            void publish_error(const Str &id, utils::Error err);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_EVENT_BUS_HPP
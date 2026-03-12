#ifndef INCLUDE_ASTRA_RP_EVENT_BUS_HPP
#define INCLUDE_ASTRA_RP_EVENT_BUS_HPP

#include <mutex>
#include <functional>
#include <type_traits>

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
            struct TokenEvent
            {
                const Str &id;
                const Str &text;
            };

            struct StateEvent
            {
                const Str &id;
                NodeState state;
            };

            struct ErrorEvent
            {
                const Str &id;
                utils::Error err;
            };

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

        private:
            template <typename Event>
            struct EventTraits;

            template <typename Event>
            using CallbackFor = typename EventTraits<std::decay_t<Event>>::Callback;

            template <typename Event>
            using SubscriberVecFor = typename EventTraits<std::decay_t<Event>>::SubscriberVec;

            template <typename Event>
            SubscriberVecFor<Event> &subscribers();

            template <typename Event>
            const SubscriberVecFor<Event> &subscribers() const;

            template <typename Event>
            static void invoke(const CallbackFor<Event> &cb, const std::decay_t<Event> &event);

        public:
            template <typename Event, typename Callback>
            void subscribe(Callback &&cb)
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                subscribers<Event>().push_back(std::forward<Callback>(cb));
            }

            template <typename Event>
            void publish(const Event &event)
            {
                SubscriberVecFor<Event> callbacks;
                {
                    std::lock_guard<std::mutex> lock(m_mtx);
                    callbacks = subscribers<Event>();
                }

                for (const auto &cb : callbacks)
                {
                    invoke<Event>(cb, event);
                }
            }

            void subscribe_token(TokenCallback cb);
            void subscribe_state(StateCallback cb);
            void subscribe_error(ErrorCallback cb);

        public:
            void publish_token(const Str &id, const Str &text);
            void publish_state(const Str &id, NodeState state);
            void publish_error(const Str &id, utils::Error err);
        };

        template <>
        struct EventBus::EventTraits<EventBus::TokenEvent>
        {
            using Callback = EventBus::TokenCallback;
            using SubscriberVec = Vec<Callback>;
        };

        template <>
        struct EventBus::EventTraits<EventBus::StateEvent>
        {
            using Callback = EventBus::StateCallback;
            using SubscriberVec = Vec<Callback>;
        };

        template <>
        struct EventBus::EventTraits<EventBus::ErrorEvent>
        {
            using Callback = EventBus::ErrorCallback;
            using SubscriberVec = Vec<Callback>;
        };

        template <typename Event>
        inline auto EventBus::subscribers() -> SubscriberVecFor<Event> &
        {
            using EventType = std::decay_t<Event>;
            if constexpr (std::is_same_v<EventType, TokenEvent>)
            {
                return m_token_subs;
            }
            else if constexpr (std::is_same_v<EventType, StateEvent>)
            {
                return m_state_subs;
            }
            else
            {
                static_assert(std::is_same_v<EventType, ErrorEvent>, "Unsupported EventBus event type");
                return m_error_subs;
            }
        }

        template <typename Event>
        inline auto EventBus::subscribers() const -> const SubscriberVecFor<Event> &
        {
            using EventType = std::decay_t<Event>;
            if constexpr (std::is_same_v<EventType, TokenEvent>)
            {
                return m_token_subs;
            }
            else if constexpr (std::is_same_v<EventType, StateEvent>)
            {
                return m_state_subs;
            }
            else
            {
                static_assert(std::is_same_v<EventType, ErrorEvent>, "Unsupported EventBus event type");
                return m_error_subs;
            }
        }

        template <typename Event>
        inline void EventBus::invoke(const CallbackFor<Event> &cb, const std::decay_t<Event> &event)
        {
            using EventType = std::decay_t<Event>;
            if constexpr (std::is_same_v<EventType, TokenEvent>)
            {
                cb(event.id, event.text);
            }
            else if constexpr (std::is_same_v<EventType, StateEvent>)
            {
                cb(event.id, event.state);
            }
            else
            {
                static_assert(std::is_same_v<EventType, ErrorEvent>, "Unsupported EventBus event type");
                cb(event.id, event.err);
            }
        }
    }
}

#endif // INCLUDE_ASTRA_RP_EVENT_BUS_HPP

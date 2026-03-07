#include "pipeline/event_bus.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        using tcb = EventBus::TokenCallback;
        using scb = EventBus::StateCallback;
        using ecb = EventBus::ErrorCallback;

        void EventBus::subscribe_token(tcb cb)
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_token_subs.push_back(cb);
        }

        void EventBus::subscribe_state(scb cb)
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_state_subs.push_back(cb);
        }

        void EventBus::subscribe_error(ecb cb)
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_error_subs.push_back(cb);
        }

        void EventBus::publish_token(const Str &id, const Str &text)
        {
            Vec<TokenCallback> callbacks;
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                callbacks = m_token_subs;
            }
            for (auto &cb : callbacks)
                cb(id, text);
        }

        void EventBus::publish_state(const Str &id, NodeState state)
        {
            Vec<StateCallback> callbacks;
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                callbacks = m_state_subs;
            }
            for (auto &cb : callbacks)
                cb(id, state);
        }

        void EventBus::publish_error(const Str &id, const Str &msg)
        {
            Vec<ErrorCallback> callbacks;
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                callbacks = m_error_subs;
            }
            for (auto &cb : callbacks)
                cb(id, msg);
        }
    }
}
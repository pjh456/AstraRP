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
            std::lock_guard<std::mutex> lock(m_mtx);
            for (auto &cb : m_token_subs)
                cb(id, text);
        }

        void EventBus::publish_state(const Str &id, NodeState state)
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            for (auto &cb : m_state_subs)
                cb(id, state);
        }

        void EventBus::publish_error(const Str &id, const Str &msg)
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            for (auto &cb : m_error_subs)
                cb(id, msg);
        }
    }
}
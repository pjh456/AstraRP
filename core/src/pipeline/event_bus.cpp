#include "pipeline/event_bus.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        void EventBus::subscribe_token(TokenCallback cb)
        {
            subscribe<TokenEvent>(std::move(cb));
        }

        void EventBus::subscribe_state(StateCallback cb)
        {
            subscribe<StateEvent>(std::move(cb));
        }

        void EventBus::subscribe_error(ErrorCallback cb)
        {
            subscribe<ErrorEvent>(std::move(cb));
        }

        void EventBus::publish_token(const Str &id, const Str &text)
        {
            publish(TokenEvent{id, text});
        }

        void EventBus::publish_state(const Str &id, NodeState state)
        {
            publish(StateEvent{id, state});
        }

        void EventBus::publish_error(const Str &id, utils::Error err)
        {
            publish(ErrorEvent{id, err});
        }
    }
}

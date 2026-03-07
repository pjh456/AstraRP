#ifndef INCLUDE_ASTRA_RP_BASE_NODE_HPP
#define INCLUDE_ASTRA_RP_BASE_NODE_HPP

#include "utils/types.hpp"
#include "pipeline/event_bus.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        struct NodePayload
        {
            Str output;
        };

        class BaseNode
        {
        protected:
            Str m_id;
            NodeState m_state = NodeState::PENDING;
            MulPtr<EventBus> m_bus;

            HashMap<Str, NodePayload> m_inputs;
            NodePayload m_output;

        public:
            BaseNode(const Str &id, MulPtr<EventBus> bus)
                : m_id(id), m_bus(bus) {}

            virtual ~BaseNode() = default;

        public:
            Str id() const noexcept { return m_id; }
            NodeState state() const noexcept { return m_state; }
            NodePayload output() const noexcept { return m_output; }

        public:
            void set_input(
                const Str &parent_id,
                const NodePayload &payload)
            {
                m_inputs[parent_id] = payload;
            }

            virtual bool execute() = 0;

        protected:
            void update_state(NodeState s)
            {
                m_state = s;
                if (m_bus)
                    m_bus->publish_state(m_id, s);
            }
        };
    }
}

#endif // INCLUDE_ASTRA_RP_BASE_NODE_HPP
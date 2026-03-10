#ifndef INCLUDE_ASTRA_RP_FORMAT_NODE_HPP
#define INCLUDE_ASTRA_RP_FORMAT_NODE_HPP

#include "utils/types.hpp"
#include "pipeline/base_node.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        class FormatNode : public BaseNode
        {
        public:
            using FormatterFunc =
                std::function<Str(const HashMap<Str, NodePayload> &)>;

        private:
            FormatterFunc m_formatter;

        public:
            FormatNode(
                const Str &id,
                MulPtr<EventBus> bus,
                FormatterFunc formatter)
                : BaseNode(id, bus),
                  m_formatter(std::move(formatter)) {}

            ResultV<void> execute() override
            {
                update_state(NodeState::RUNNING);

                if (m_formatter)
                {
                    m_output.output = m_formatter(m_inputs);
                }
                else
                {
                    if (!m_inputs.empty())
                    {
                        m_output.output = m_inputs.begin()->second.output;
                    }
                }

                update_state(NodeState::FINISHED);

                return ResultV<void>::Ok();
            }
        };
    }
}

#endif // INCLUDE_ASTRA_RP_FORMAT_NODE_HPP
#ifndef INCLUDE_ASTRA_RP_FORMAT_NODE_HPP
#define INCLUDE_ASTRA_RP_FORMAT_NODE_HPP

#include "utils/types.hpp"
#include <algorithm>
#include "pipeline/base_node.hpp"
#include "utils/logger.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        class FormatNode : public BaseNode
        {
        public:
            struct FormatPart
            {
                enum class Type
                {
                    TEXT,
                    NODE
                };

                Type type = Type::TEXT;
                Str value;
            };

            using FormatterFunc =
                std::function<Str(const HashMap<Str, NodePayload> &)>;

            static Str apply_format(
                const Vec<FormatPart> &parts,
                const HashMap<Str, NodePayload> &inputs)
            {
                Str output;
                for (const auto &part : parts)
                {
                    if (part.type == FormatPart::Type::TEXT)
                    {
                        output += part.value;
                        continue;
                    }

                    auto it = inputs.find(part.value);
                    if (it != inputs.end())
                    {
                        output += it->second.output;
                    }
                }

                return output;
            }

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

                if (m_bus)
                {
                    ASTRA_LOG_DEBUG("FormatNode " + m_id + " input count=" + std::to_string(m_inputs.size()));
                    for (const auto &[parent_id, payload] : m_inputs)
                    {
                        ASTRA_LOG_DEBUG("FormatNode " + m_id + " input[" + parent_id + "]=" + payload.output);
                    }
                }

                if (m_formatter)
                {
                    m_output.output = m_formatter(m_inputs);
                }
                else
                {
                    if (!m_inputs.empty())
                    {
                        Vec<Str> parent_ids;
                        parent_ids.reserve(m_inputs.size());
                        for (const auto &[parent_id, _] : m_inputs)
                        {
                            parent_ids.push_back(parent_id);
                        }
                        std::sort(parent_ids.begin(), parent_ids.end());

                        Str merged;
                        for (const auto &parent_id : parent_ids)
                        {
                            merged += m_inputs.at(parent_id).output;
                        }
                        m_output.output = merged;
                    }
                }

                if (m_bus)
                {
                    ASTRA_LOG_DEBUG("FormatNode " + m_id + " output=" + m_output.output);
                }

                if (m_bus)
                {
                    for (const char ch : m_output.output)
                    {
                        Str token(1, ch);
                        m_bus->publish_token(m_id, token);
                    }
                }

                update_state(NodeState::FINISHED);

                return ResultV<void>::Ok();
            }
        };
    }
}

#endif // INCLUDE_ASTRA_RP_FORMAT_NODE_HPP

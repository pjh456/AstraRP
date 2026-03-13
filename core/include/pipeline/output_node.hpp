#ifndef INCLUDE_ASTRA_RP_OUTPUT_NODE_HPP
#define INCLUDE_ASTRA_RP_OUTPUT_NODE_HPP

#include <mutex>

#include "utils/types.hpp"
#include <algorithm>
#include "pipeline/base_node.hpp"
#include "utils/logger.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        class OutputNode : public BaseNode
        {
        private:
            Str m_accumulated_content;
            mutable std::mutex m_mtx; // 保护状态的读写安全

        public:
            OutputNode(
                const Str &id,
                MulPtr<EventBus> bus)
                : BaseNode(id, bus) {}

        public:
            void clear()
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                m_accumulated_content.clear();
                m_output.output.clear();
            }

            Str content() const
            {
                std::lock_guard<std::mutex> lock(m_mtx);
                return m_accumulated_content;
            }

        public:
            ResultV<void> execute() override
            {
                update_state(NodeState::RUNNING);

                std::lock_guard<std::mutex> lock(m_mtx);
                m_output.output.clear();

                ASTRA_LOG_DEBUG("OutputNode " + m_id + " input count=" + std::to_string(m_inputs.size()));

                Vec<Str> parent_ids;
                parent_ids.reserve(m_inputs.size());
                for (const auto &[parent_id, payload] : m_inputs)
                {
                    (void)payload;
                    parent_ids.push_back(parent_id);
                }
                std::sort(parent_ids.begin(), parent_ids.end());

                // 将所有上游连接的数据按稳定顺序拼接，作为自己的输出
                for (const auto &parent_id : parent_ids)
                {
                    const auto &payload = m_inputs.at(parent_id);
                    ASTRA_LOG_DEBUG("OutputNode " + m_id + " input[" + parent_id + "]=" + payload.output);
                    m_output.output += payload.output;
                }

                // 保存当前输出快照
                m_accumulated_content = m_output.output;
                ASTRA_LOG_DEBUG("OutputNode " + m_id + " output=" + m_output.output);

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

#endif // INCLUDE_ASTRA_RP_OUTPUT_NODE_HPP
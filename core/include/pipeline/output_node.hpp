#ifndef INCLUDE_ASTRA_RP_OUTPUT_NODE_HPP
#define INCLUDE_ASTRA_RP_OUTPUT_NODE_HPP

#include <mutex>

#include "utils/types.hpp"
#include "pipeline/base_node.hpp"

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

                // 将所有上游连接的数据拼接，作为自己的输出
                for (const auto &[parent_id, payload] : m_inputs)
                {
                    m_output.output += payload.output;
                }

                // 截取并持久化保存
                m_accumulated_content += m_output.output;

                update_state(NodeState::FINISHED);
                return ResultV<void>::Ok();
            }
        };
    }
}

#endif // INCLUDE_ASTRA_RP_OUTPUT_NODE_HPP
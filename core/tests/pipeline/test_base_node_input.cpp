#include <iostream>

#include "pipeline/base_node.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        class InputNode final : public BaseNode
        {
        private:
            Str m_parent_id;

        public:
            InputNode(const Str &id, const Str &parent_id)
                : BaseNode(id, nullptr), m_parent_id(parent_id) {}

            ResultV<void> execute() override
            {
                auto it = m_inputs.find(m_parent_id);
                if (it == m_inputs.end())
                {
                    return ResultV<void>::Err(
                        utils::ErrorBuilder()
                            .pipeline()
                            .missing_dependency()
                            .message("missing input")
                            .build());
                }

                if (it->second.output != "payload")
                {
                    return ResultV<void>::Err(
                        utils::ErrorBuilder()
                            .pipeline()
                            .invalid_argument()
                            .message("payload mismatch")
                            .build());
                }

                return ResultV<void>::Ok();
            }
        };
    }
}

int main()
{
    using namespace astra_rp;

    pipeline::InputNode node("B", "A");
    pipeline::NodePayload payload;
    payload.output = "payload";
    node.set_input("A", payload);

    auto res = node.execute();
    if (res.is_err())
    {
        std::cerr << "BaseNode input storage failed." << std::endl;
        return 1;
    }

    std::cout << "BaseNode input storage test passed." << std::endl;
    return 0;
}

#include "infer/tokenize_task.hpp"

#include "core/model.hpp"
#include "infer/engine.hpp"

namespace astra_rp
{
    namespace infer
    {
        ResultV<void>
        TokenizeTask::execute()
        {
            ASSIGN_OR_RETURN(
                tokens,
                Engine::instance()
                    .str2token(
                        m_model,
                        m_prompt)
                    .map_err(
                        [this](auto err)
                        {
                            return utils::ErrorBuilder()
                                .infer()
                                .tokenize_failed()
                                .message("Tokenize failed!")
                                .context_id(m_model->name())
                                .wrap(std::move(err))
                                .build();
                        }));

            m_tokens = std::move(tokens);

            return ResultV<void>::Ok();
        }
    }
}
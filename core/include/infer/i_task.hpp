#ifndef INCLUDE_ASTRA_RP_I_TASK_HPP
#define INCLUDE_ASTRA_RP_I_TASK_HPP

#include "utils/types.hpp"

namespace astra_rp
{
    namespace infer
    {
        class ITask
        {
        public:
            virtual ~ITask() = default;

            virtual Str name() const noexcept = 0;

            virtual ResultV<void> execute() = 0;
        };
    }
}

#endif // INCLUDE_ASTRA_RP_I_TASK_HPP
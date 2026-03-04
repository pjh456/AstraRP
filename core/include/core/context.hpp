#ifndef INCLUDE_ASTRA_RP_CONTEXT_HPP
#define INCLUDE_ASTRA_RP_CONTEXT_HPP

#include "utils/types.hpp"

struct llama_context;

namespace astra_rp
{
    namespace core
    {
        class Model;
        class ContextManager;

        class Context
        {
            friend class ContextManager;

        private:
            llama_context *m_ctx;
            MulPtr<Model> m_model;

        private:
            Context(MulPtr<Model> model, llama_context *ctx = nullptr);

        public:
            ~Context();

            Context(const Context &) = delete;
            Context &operator=(const Context &) = delete;

            Context(Context &&) noexcept = default;
            Context &operator=(Context &&) noexcept = default;

        public:
            llama_context *raw() const noexcept { return m_ctx; }
            const Model &model() const noexcept { return *m_model; }

        public:
            uint32_t capacity() const;

        public:
            // 清空当前上下文中的所有内存与缓存
            void clear(bool only_meta);

            // 获取指定 seq_id (小于 0 则全局) 所占用的状态字节数
            size_t state_size(int32_t seq_id = -1) const;

            // 获取指定 seq_id (小于 0 则全局) 的状态数据 (序列化)
            Vec<uint8_t> get_state(int32_t seq_id = -1) const;

            // 将状态数据恢复并覆盖到指定的 seq_id (小于 0 则全局)
            size_t set_state(const Vec<uint8_t> &data, int32_t seq_id = -1);
        };
    }
}

#endif // INCLUDE_ASTRA_RP_CONTEXT_HPP
#ifndef INCLUDE_ASTRA_RP_MODEL_HPP
#define INCLUDE_ASTRA_RP_MODEL_HPP

#include "utils/types.hpp"

struct llama_model;
struct llama_context;
struct llama_batch;

namespace astra_rp
{
    namespace core
    {
        /**
         * @brief Model 类封装了 llama_model 的生命周期。
         * 它负责：
         * 1. 全局 Backend 的初始化
         * 2. GGUF 模型文件的加载与释放
         * 3. 文本到 Token 的转换 (Tokenization)
         */
        class Model
        {
        public:
            using Ptr = std::shared_ptr<Model>;

        private:
            llama_model *m_model = nullptr;
            Str m_path;

        private:
            // 确保 llama_backend_init 只被调用一次
            static void init_backend();

        public:
            explicit Model(const std::string &path);

            Model(const Model &) = delete;
            Model &operator=(const Model &) = delete;

            Model(Model &&) noexcept = default;
            Model &operator=(Model &&) noexcept = default;

            ~Model();

        public:
            /**
             * @brief 将文本转换为 token ID 列表
             * @param text 输入文本
             * @param add_bos 是否添加 Begin-Of-Sentence token
             * @param special 是否允许特殊 token (如 <|endoftext|>)
             */
            Vec<int> tokenize(const Str &text, bool add_bos = true, bool special = false) const;

            /**
             * @brief 将 token ID 转换回文本 (用于调试或流式输出)
             */
            Str detokenize(int token_id) const;
        };
    } // namespace core
} // namespace astra_rp

#endif // INCLUDE_ASTRA_RP_MODEL_HPP
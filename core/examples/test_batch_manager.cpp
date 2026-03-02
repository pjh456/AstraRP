#include <iostream>
#include <vector>
#include <cassert>

#include "core/batch_manager.hpp"
#include "core/batch.hpp"

int main()
{
    using namespace astra_rp::core;
    std::cout << "Batch Test started." << std::endl;

    // 1. 初始化 Manager
    auto &manager = BatchManager::instance();
    std::cout << "Batch Manager successfully initialized." << std::endl;

    // 2. 从池中获取一个 Batch
    int32_t test_capacity = 512;
    auto batch = manager.acquire(test_capacity);
    std::cout << "Batch successfully acquired from manager." << std::endl;

    // 3. 基础属性检查
    assert(batch != nullptr);
    assert(batch->capacity() >= test_capacity);
    assert(batch->size() == 0); // 初始大小应为 0
    std::cout << "Initial state check passed. Capacity: " << batch->capacity() << std::endl;

    // 4. 填充数据检查 (addToken)
    llama_token test_token_id = 15496; // 假设的 token id
    int32_t test_pos = 10;
    std::vector<int32_t> test_seq_ids = {0};
    bool test_logits = true;

    batch->add(test_token_id, test_pos, test_seq_ids, test_logits);

    assert(batch->size() == 1);
    // 检查底层 C 结构体数据是否正确映射
    assert(batch->raw().token[0] == test_token_id);
    assert(batch->raw().pos[0] == test_pos);
    assert(batch->raw().logits[0] == test_logits);
    assert(batch->raw().n_seq_id[0] == (int32_t)test_seq_ids.size());
    std::cout << "Data integrity check passed (Token added)." << std::endl;

    // 5. 重置检查 (clear)
    batch->clear();
    assert(batch->size() == 0);
    std::cout << "Clear logic check passed." << std::endl;

    // 6. 对象池复用检查
    // 记录当前 batch 的原始地址
    void *original_addr = static_cast<void *>(batch.get());

    // 让当前 batch 离开作用域（或者手动 reset），触发自定义删除器将其还给池
    batch.reset();
    std::cout << "Batch returned to pool." << std::endl;

    // 再次申请相同容量的 batch
    auto batch_reused = manager.acquire(test_capacity);
    void *reused_addr = static_cast<void *>(batch_reused.get());

    // 验证地址是否一致（验证复用逻辑）
    assert(original_addr == reused_addr);
    assert(batch_reused->size() == 0); // 复用时应该已被自动 clear
    std::cout << "Pool reuse check passed (Address: " << reused_addr << ")." << std::endl;

    // 7. 边界异常测试 (可选)
    try
    {
        // 如果 add 实现了越界检查，可以测试这个
        for (int i = 0; i < 1000; ++i)
            batch_reused->add(1, i, {0}, false);
    }
    catch (const std::exception &e)
    {
        std::cout << "Boundary check caught expected exception: " << e.what() << std::endl;
    }

    std::cout << "All Batch tests passed successfully." << std::endl;
    return 0;
}
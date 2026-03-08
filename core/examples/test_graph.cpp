#include <iostream>
#include <cassert>

#define ASTRA_SIMPLE_LOG

#include "utils/logger.hpp"
#include "pipeline/graph.hpp"
#include "pipeline/base_node.hpp"

using namespace astra_rp;
using namespace astra_rp::pipeline;

// ---------------------------------------------------------
// 仅用于填充 Graph 拓扑结构的空节点
// ---------------------------------------------------------
class DummyNode : public BaseNode
{
public:
    DummyNode(const Str &id) : BaseNode(id, nullptr) {}
    bool execute() override { return true; } // Graph测试不调用它
};

// ---------------------------------------------------------
// 测试 1: 合法 DAG (无环图)
// ---------------------------------------------------------
void test_valid_acyclic_graph()
{
    Graph graph;

    // 构建拓扑: A -> B -> D
    //          \-> C -/
    graph.add_node(MulPtr<DummyNode>(new DummyNode("A")));
    graph.add_node(MulPtr<DummyNode>(new DummyNode("B")));
    graph.add_node(MulPtr<DummyNode>(new DummyNode("C")));
    graph.add_node(MulPtr<DummyNode>(new DummyNode("D")));

    graph.add_edge("A", "B");
    graph.add_edge("A", "C");
    graph.add_edge("B", "D");
    graph.add_edge("C", "D");

    assert(graph.validate().is_ok());
    ASTRA_LOG_INFO("Acyclic graph successfully validated.");
}

// ---------------------------------------------------------
// 测试 2: 非法图 (存在死锁环路)
// ---------------------------------------------------------
void test_invalid_cyclic_graph()
{
    Graph graph;

    graph.add_node(MulPtr<DummyNode>(new DummyNode("X")));
    graph.add_node(MulPtr<DummyNode>(new DummyNode("Y")));
    graph.add_node(MulPtr<DummyNode>(new DummyNode("Z")));

    // 构造死锁环: X -> Y -> Z -> X
    graph.add_edge("X", "Y");
    graph.add_edge("Y", "Z");
    graph.add_edge("Z", "X");

    assert(graph.validate().is_err());
    ASTRA_LOG_INFO("Cyclic graph (cycle detected) successfully rejected.");
}

// ---------------------------------------------------------
// 测试 3: 多起点和断开的图 (合法)
// ---------------------------------------------------------
void test_disconnected_graph()
{
    Graph graph;

    // 子图 1: A -> B
    graph.add_node(MulPtr<DummyNode>(new DummyNode("A")));
    graph.add_node(MulPtr<DummyNode>(new DummyNode("B")));
    graph.add_edge("A", "B");

    // 子图 2: 孤立点 C
    graph.add_node(MulPtr<DummyNode>(new DummyNode("C")));

    // 只要没有环，不管有几个起点，都是合法的 DAG
    assert(graph.validate().is_ok());
    ASTRA_LOG_INFO("Disconnected graph successfully validated.");
}

int main()
{
    std::cout << "========== STARTING PIPELINE_GRAPH TESTS ==========" << std::endl;
    try
    {
        test_valid_acyclic_graph();
        test_invalid_cyclic_graph();
        test_disconnected_graph();
    }
    catch (const std::exception &e)
    {
        std::cerr << "[FATAL] Graph Test failed: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "========== ALL PIPELINE_GRAPH TESTS PASSED ==========" << std::endl;
    return 0;
}
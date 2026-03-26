#include "pipeline/graph.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        void Graph::add_node(MulPtr<BaseNode> node)
        {
            m_nodes[node->id()] = node;
            if (m_in_degrees.find(node->id()) == m_in_degrees.end())
                m_in_degrees[node->id()] = 0;
        }

        void Graph::add_edge(const Str &from_id, const Str &to_id)
        {
            m_link_table[from_id].push_back(to_id);
            m_in_degrees[to_id]++;
        }

        ResultV<void>
        Graph::validate() const
        {
            if (m_nodes.empty())
                return ResultV<void>::Ok();

            for (const auto &[from_id, targets] : m_link_table)
            {
                if (m_nodes.find(from_id) == m_nodes.end())
                {
                    return ResultV<void>::Err(
                        utils::ErrorBuilder()
                            .pipeline()
                            .graph_cycle_detected()
                            .message("Graph validation failed. Edge source node not found: " + from_id)
                            .build());
                }
                for (const auto &to_id : targets)
                {
                    if (m_nodes.find(to_id) == m_nodes.end())
                    {
                        return ResultV<void>::Err(
                            utils::ErrorBuilder()
                                .pipeline()
                                .graph_cycle_detected()
                                .message("Graph validation failed. Edge target node not found: " + to_id)
                                .build());
                    }
                }
            }

            HashMap<Str, int> in_degrees_copy = m_in_degrees;

            Queue<Str> zero_in_degree_queue;
            for (const auto &[k, v] : in_degrees_copy)
                if (v == 0)
                    zero_in_degree_queue.push(k);

            if (zero_in_degree_queue.empty())
                return ResultV<void>::Err(
                    utils::ErrorBuilder()
                        .pipeline()
                        .graph_cycle_detected()
                        .message("Graph validation failed. No starting node with zero in-degree found, indicating a potential cycle or invalid topology.")
                        .build());

            int processed_count = 0;

            while (!zero_in_degree_queue.empty())
            {
                auto cur_node = zero_in_degree_queue.front();
                zero_in_degree_queue.pop();

                processed_count++;

                auto it = m_link_table.find(cur_node);
                if (it == m_link_table.end())
                    continue;

                for (const auto &neighbor : it->second)
                {
                    in_degrees_copy[neighbor]--;
                    if (in_degrees_copy[neighbor] == 0)
                        zero_in_degree_queue.push(neighbor);
                }
            }

            if (processed_count != m_nodes.size())
                return ResultV<void>::Err(
                    utils::ErrorBuilder()
                        .pipeline()
                        .graph_cycle_detected()
                        .message("Graph validation failed. A cyclic dependency or invalid topology was detected.")
                        .build());
            return ResultV<void>::Ok();
        }
    }
}

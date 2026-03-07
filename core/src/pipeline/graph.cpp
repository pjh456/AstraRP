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

        bool Graph::validate() const
        {
            if (m_nodes.empty())
                return true;

            HashMap<Str, int> in_degrees_copy = m_in_degrees;

            Queue<Str> zero_in_degree_queue;
            for (const auto &[k, v] : in_degrees_copy)
                if (v == 0)
                    zero_in_degree_queue.push(k);

            if (zero_in_degree_queue.empty())
                return false;

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

            return processed_count == m_nodes.size();
        }
    }
}
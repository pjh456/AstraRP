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
    }
}
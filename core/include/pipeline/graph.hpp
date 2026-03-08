#ifndef INCLUDE_ASTRA_RP_GRAPH_HPP
#define INCLUDE_ASTRA_RP_GRAPH_HPP

#include "utils/types.hpp"
#include "pipeline/base_node.hpp"

namespace astra_rp
{
    namespace pipeline
    {
        class Graph
        {
        private:
            HashMap<Str, MulPtr<BaseNode>> m_nodes;
            HashMap<Str, Vec<Str>> m_link_table;
            HashMap<Str, int> m_in_degrees;

        public:
            const auto &nodes() const { return m_nodes; }
            const auto &link_table() const { return m_link_table; }
            const auto &in_degrees() const { return m_in_degrees; }

        public:
            void add_node(MulPtr<BaseNode> node);

            void add_edge(const Str &from_id, const Str &to_id);

            ResultV<void>
            validate() const;
        };
    }
}

#endif // INCLUDE_ASTRA_RP_GRAPH_HPP
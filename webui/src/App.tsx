import { useState, useMemo } from 'react';
import {
  ReactFlow,
  Controls,
  Background,
  useNodesState,
  useEdgesState,
  addEdge
} from '@xyflow/react';

// 强制引入 React Flow 的核心样式，防止 CSS 丢失
import '@xyflow/react/dist/style.css';

import InferenceNode from './nodes/InferenceNode';
import FormatNode from './nodes/FormatNode';
import Sidebar from './components/Sidebar';

const initNodes = [
  {
    id: 'format_1',
    type: 'formatNode',
    position: { x: 100, y: 150 },
    data: { formatStr: 'User: Hello\\nAssistant:' }
  },
  {
    id: 'infer_1',
    type: 'inferenceNode',
    position: { x: 450, y: 130 },
    data: { model: 'qwen2.5-0.5b', maxTokens: 150, temperature: 0.7 }
  }
];

const initEdges = [
  {
    id: 'e1-2',
    source: 'format_1',
    target: 'infer_1',
    animated: true,
    style: { stroke: '#a855f7', strokeWidth: 2 }
  }
];

export default function App() {
  const [nodes, setNodes, onNodesChange] = useNodesState(initNodes);
  const [edges, setEdges, onEdgesChange] = useEdgesState(initEdges);
  // 使用 any 避开严格的 TS 泛型检查导致的网页截断 Bug
  const [selectedNode, setSelectedNode] = useState<any>(null);

  const nodeTypes = useMemo(() => {
    return {
      inferenceNode: InferenceNode,
      formatNode: FormatNode
    };
  }, []);

  const onConnect = (params: any) => {
    const newEdge = { ...params, animated: true, style: { stroke: '#a855f7', strokeWidth: 2 } };
    setEdges((eds) => addEdge(newEdge, eds));
  };

  const handleSelectionChange = (params: any) => {
    if (params.nodes && params.nodes.length > 0) {
      setSelectedNode(params.nodes[0]);
    } else {
      setSelectedNode(null);
    }
  };

  return (
    // 加入了 style={{ width: '100vw', height: '100vh' }} 保证绝对有高度
    <div className="flex bg-gray-900" style={{ width: '100vw', height: '100vh' }}>
      <Sidebar selectedNode={selectedNode} />

      <div className="flex-1 h-full relative">
        <ReactFlow
          nodes={nodes}
          edges={edges}
          onNodesChange={onNodesChange}
          onEdgesChange={onEdgesChange}
          onConnect={onConnect}
          nodeTypes={nodeTypes}
          onSelectionChange={handleSelectionChange}
          fitView
        >
          <Background color="#374151" />
          <Controls />
        </ReactFlow>
      </div>
    </div>
  );
}
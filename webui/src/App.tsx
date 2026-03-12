import { useState, useMemo, useEffect, useRef } from 'react';
import {
  ReactFlow,
  Controls,
  Background,
  useNodesState,
  useEdgesState,
  addEdge
} from '@xyflow/react';
import type {
  Node,
  Edge,
  Connection,
  OnSelectionChangeParams
} from '@xyflow/react';
import '@xyflow/react/dist/style.css';

import InferenceNode from './nodes/InferenceNode';
import FormatNode from './nodes/FormatNode';
import OutputNode from './nodes/OutputNode';
import Sidebar from './components/Sidebar';
import TokenEdge from './edges/TokenEdge';

type FormatNodeData = {
  formatStr: string;
};

type InferenceNodeData = {
  model: string;
  maxTokens: number;
  temperature: number;
};

type OutputNodeData = {
  text: string;
};

type AppNode =
  | Node<FormatNodeData>
  | Node<InferenceNodeData>
  | Node<OutputNodeData>;

type TokenEdgeData = {
  tokens: string[];
};

type AppEdge = Edge<TokenEdgeData>;

const initNodes: AppNode[] = [
  {
    id: 'format_1',
    type: 'formatNode',
    position: { x: 50, y: 150 },
    data: { formatStr: 'User: Hello\\nAssistant:' }
  },
  {
    id: 'infer_1',
    type: 'inferenceNode',
    position: { x: 350, y: 130 },
    data: { model: 'qwen2.5-0.5b', maxTokens: 150, temperature: 0.7 }
  },
  {
    id: 'out_1',
    type: 'outputNode',
    position: { x: 700, y: 130 },
    data: { text: '' }
  }
];

const initEdges = [
  {
    id: 'e1-2',
    source: 'format_1',
    target: 'infer_1',
    type: 'tokenEdge',
    style: { stroke: '#a855f7', strokeWidth: 2, strokeDasharray: '5,5' },
    data: { tokens: [] }
  },
  {
    id: 'e2-3',
    source: 'infer_1',
    target: 'out_1',
    type: 'tokenEdge',
    style: { stroke: '#22c55e', strokeWidth: 2, strokeDasharray: '5,5' },
    data: { tokens: [] }
  }
];

export default function App() {
  const [nodes, setNodes, onNodesChange] = useNodesState<AppNode>(initNodes);
  const [edges, setEdges, onEdgesChange] = useEdgesState<AppEdge>(initEdges);
  const [selectedNode, setSelectedNode] = useState<AppNode | null>(null);
  const [isRunning, setIsRunning] = useState(false);

  // 引用缓冲，用于积累文本，避免高频 React 渲染卡顿
  const outputBuffer = useRef('');

  const nodeTypes = useMemo(
    () => ({
      inferenceNode: InferenceNode,
      formatNode: FormatNode,
      outputNode: OutputNode
    }),
    []
  );
  const edgeTypes = useMemo(
    () => ({ tokenEdge: TokenEdge }),
    []
  );

  const handleIncomingToken = (nodeId: string, char: string) => {
    // 1. 累加到缓冲器
    outputBuffer.current += char;

    // 2. 更新 Output 节点显示
    setNodes((nds) =>
      nds.map((node) => {
        if (node.id === 'out_1') {
          const data = node.data as OutputNodeData;

          return {
            ...node,
            data: { ...data, text: outputBuffer.current }
          };
        }
        return node;
      })
    );

    // 3. 触发连线上的 "流光文字" 效果
    setEdges((eds) =>
      eds.map((edge) => {
        if (edge.source === nodeId) {
          const tokens = edge.data?.tokens ?? [];

          return {
            ...edge,
            data: {
              tokens: [...tokens, char]
            }
          };
        }
        return edge;
      })
    );
  };

  // 模拟从 C++ / Node.js 服务器不断推来 Token
  useEffect(() => {
    const mockText = "Hello! I am Astra RP. This is a streaming token test with cyber-edge animation effect. Everything looks perfect.";
    let i = 0;

    // 每 150ms 模拟收到一个后端吐出的 token
    const timer = setInterval(() => {
      if (i < mockText.length) {
        // 假设这里你调用了 await astra.detokenize('default', [token_id])
        const char = mockText[i];
        handleIncomingToken('infer_1', char);
        i++;
      }
    }, 150);

    return () => clearInterval(timer);
  }, []);

  const onConnect = (params: Connection) => {
    const newEdge: AppEdge = {
      ...params,
      id: `${params.source}-${params.target}`,
      type: 'tokenEdge',
      animated: true,
      style: { stroke: '#a855f7', strokeWidth: 2 },
      data: { tokens: [] }
    };
    setEdges((eds) => addEdge(newEdge, eds));
  };

  const handleSelectionChange = (params: OnSelectionChangeParams) => {
    setSelectedNode(params.nodes?.[0] as AppNode || null);
  };

  const runPipeline = async () => {
    if (isRunning) return;
    setIsRunning(true);

    // 清空前端缓存与界面状态
    outputBuffer.current = '';
    setNodes((nds) => nds.map(n => n.id === 'out_1' ? { ...n, data: { text: '' } } : n));
    setEdges((eds) => eds.map(e => ({ ...e, data: { tokens: [] } })));

    try {
      // 提取 FormatNode 里的 prompt 作为参数
      const formatNode = nodes.find(n => n.id === 'format_1') as Node<FormatNodeData>;
      const formatStr = formatNode?.data?.formatStr || "User: Hello\nAssistant:";

      // 向我们刚才写的 Node.js 桥接服务器发起请求
      const response = await fetch('http://localhost:3000/api/run', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ formatStr })
      });

      if (!response.body) throw new Error("ReadableStream not supported");

      // 处理流式响应 (NDJSON 解析)
      const reader = response.body.getReader();
      const decoder = new TextDecoder('utf-8');
      let chunkBuffer = ''; // 用于处理被截断的 JSON 行

      while (true) {
        const { done, value } = await reader.read();
        if (done) break;

        chunkBuffer += decoder.decode(value, { stream: true });
        const lines = chunkBuffer.split('\n');

        // 留着最后一行（可能是不完整的 JSON）到下个循环处理
        chunkBuffer = lines.pop() || '';

        for (const line of lines) {
          if (!line.trim()) continue;
          const data = JSON.parse(line);

          if (data.error) {
            console.error("Backend Error:", data.error);
            alert("Error: " + data.error);
          } else if (data.done) {
            console.log("Pipeline execution finished!");
          } else if (data.text) {
            // 将真实的 Token 喂给我们的更新函数！
            handleIncomingToken(data.nodeId, data.text);
          }
        }
      }
    } catch (err) {
      console.error("Run pipeline failed:", err);
    } finally {
      setIsRunning(false);
    }
  };

  return (
    <div className="flex bg-gray-900" style={{ width: '100vw', height: '100vh' }}>
      <Sidebar
        selectedNode={selectedNode}
        onRun={runPipeline}
        isRunning={isRunning}
      />
      <div className="flex-1 h-full relative">
        <ReactFlow
          nodes={nodes}
          edges={edges}
          onNodesChange={onNodesChange}
          onEdgesChange={onEdgesChange}
          onConnect={onConnect}
          nodeTypes={nodeTypes}
          edgeTypes={edgeTypes} // [应用自定义连线]
          onSelectionChange={handleSelectionChange}
          fitView
          proOptions={{ hideAttribution: true }}
        >
          <Background color="#374151" />
          <Controls />
        </ReactFlow>
      </div>
    </div>
  );
}
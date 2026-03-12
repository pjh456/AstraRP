import { useState, useMemo, useRef } from 'react';
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
import LogSidebar from './components/LogSidebar';
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

type NodeKind = 'formatNode' | 'inferenceNode' | 'outputNode';

const defaultNodeData: Record<NodeKind, FormatNodeData | InferenceNodeData | OutputNodeData> = {
  formatNode: { formatStr: 'User: Hello\\nAssistant:' },
  inferenceNode: { model: 'qwen2.5-0.5b', maxTokens: 150, temperature: 0.7 },
  outputNode: { text: '' }
};

const makeNodeId = (type: NodeKind) => `${type}_${Math.random().toString(36).slice(2, 10)}`;

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
  const abortControllerRef = useRef<AbortController | null>(null);
  const [contextMenu, setContextMenu] = useState<{ x: number; y: number } | null>(null);

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
    outputBuffer.current += char;

    const targetOutputIds = new Set(
      edges
        .filter((edge) => edge.source === nodeId)
        .map((edge) => edge.target)
    );

    setNodes((nds) =>
      nds.map((node) => {
        if (targetOutputIds.has(node.id) && node.type === 'outputNode') {
          const data = node.data as OutputNodeData;

          return {
            ...node,
            data: { ...data, text: outputBuffer.current }
          };
        }
        return node;
      })
    );

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
  /*
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
  */

  const onConnect = (params: Connection) => {
    if (isRunning) return;

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

  const addNodeByContextMenu = (type: NodeKind) => {
    if (!contextMenu || isRunning) return;

    const newNode: AppNode = {
      id: makeNodeId(type),
      type,
      position: { x: contextMenu.x, y: contextMenu.y },
      data: structuredClone(defaultNodeData[type])
    } as AppNode;

    setNodes((nds) => [...nds, newNode]);
    setContextMenu(null);
  };

  const deleteNode = (nodeId: string) => {
    if (isRunning) return;
    setNodes((nds) => nds.filter((node) => node.id !== nodeId));
    setEdges((eds) => eds.filter((edge) => edge.source !== nodeId && edge.target !== nodeId));
    setSelectedNode(null);
  };

  const saveNode = (nodeId: string, nextData: Record<string, string | number>) => {
    if (isRunning) return;

    const currentNode = nodes.find((node) => node.id === nodeId);
    if (!currentNode) return;

    const hasChanges = JSON.stringify(currentNode.data) !== JSON.stringify(nextData);
    if (!hasChanges) return;

    const newNodeId = makeNodeId(currentNode.type as NodeKind);
    const recreatedNode = {
      ...currentNode,
      id: newNodeId,
      data: nextData
    } as AppNode;

    setNodes((nds) => nds.map((node) => (node.id === nodeId ? recreatedNode : node)));
    setEdges((eds) =>
      eds.map((edge) => ({
        ...edge,
        source: edge.source === nodeId ? newNodeId : edge.source,
        target: edge.target === nodeId ? newNodeId : edge.target,
        id: `${edge.source === nodeId ? newNodeId : edge.source}-${edge.target === nodeId ? newNodeId : edge.target}`
      }))
    );
    setSelectedNode(recreatedNode);
  };

  const handleSelectionChange = (params: OnSelectionChangeParams) => {
    setSelectedNode(params.nodes?.[0] as AppNode || null);
  };

  const runPipeline = async () => {
    if (isRunning) return;
    setIsRunning(true);
    abortControllerRef.current = new AbortController();

    outputBuffer.current = '';
    setNodes((nds) => nds.map((n) => (n.type === 'outputNode' ? { ...n, data: { text: '' } } : n)));
    setEdges((eds) => eds.map(e => ({ ...e, data: { tokens: [] } })));

    try {
      const formatNode = nodes.find(n => n.type === 'formatNode') as Node<FormatNodeData>;
      const formatStr = formatNode?.data?.formatStr || "User: Hello\nAssistant:";

      // 向我们刚才写的 Node.js 桥接服务器发起请求
      const response = await fetch('http://localhost:3000/api/run', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ formatStr }),
        signal: abortControllerRef.current.signal
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

          let data;
          try {
            data = JSON.parse(line); // 加入 try-catch 防止报错断开流
          } catch {
            console.warn("Skipping invalid JSON chunk:", line);
            continue;
          }

          if (data.error) {
            console.error("Backend Error:", data.error);
            alert("Engine Error: " + data.error); // 暴露后台 OOM 等错误给用户
          } else if (data.done) {
            console.log("Pipeline execution finished!");
          } else if (data.text) {
            // 将真实的 Token 喂给更新函数！
            handleIncomingToken(data.nodeId, data.text);
          }
        }
      }
    } catch (err: unknown) {
      if (err instanceof Error && err.name === 'AbortError') {
        console.log("Pipeline stopped by user");
      } else {
        console.error("Run pipeline failed:", err);
      }
    } finally {
      setIsRunning(false);
    }
  };

  const stopPipeline = () => {
    if (abortControllerRef.current) {
      abortControllerRef.current.abort();
    }
  };

  return (
    <div className="flex bg-gray-900" style={{ width: '100vw', height: '100vh' }}>
      <Sidebar
        selectedNode={selectedNode}
        onDeleteNode={deleteNode}
        onSaveNode={saveNode}
        onRun={runPipeline}
        onStop={stopPipeline}
        isRunning={isRunning}
      />
      <div className="flex-1 h-full relative" onClick={() => setContextMenu(null)}>
        <ReactFlow
          nodes={nodes}
          edges={edges}
          onNodesChange={onNodesChange}
          onEdgesChange={onEdgesChange}
          onConnect={onConnect}
          nodeTypes={nodeTypes}
          edgeTypes={edgeTypes} // [应用自定义连线]
          onSelectionChange={handleSelectionChange}
          onPaneContextMenu={(event) => {
            event.preventDefault();
            if (isRunning) return;
            setContextMenu({ x: event.clientX - 320, y: event.clientY });
          }}
          fitView
          proOptions={{ hideAttribution: true }}
        >
          <Background color="#374151" />
          <Controls />
        </ReactFlow>

        {contextMenu && (
          <div
            className="absolute z-10 bg-gray-800 border border-gray-700 rounded-md shadow-xl min-w-[180px]"
            style={{ left: contextMenu.x, top: contextMenu.y }}
          >
            <button onClick={() => addNodeByContextMenu('formatNode')} className="w-full text-left px-3 py-2 text-sm text-gray-200 hover:bg-gray-700">添加 Format 节点</button>
            <button onClick={() => addNodeByContextMenu('inferenceNode')} className="w-full text-left px-3 py-2 text-sm text-gray-200 hover:bg-gray-700">添加 Inference 节点</button>
            <button onClick={() => addNodeByContextMenu('outputNode')} className="w-full text-left px-3 py-2 text-sm text-gray-200 hover:bg-gray-700">添加 Output 节点</button>
          </div>
        )}
      </div>
      <LogSidebar />
    </div>
  );
}

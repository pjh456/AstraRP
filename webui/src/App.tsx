import { useState, useMemo, useRef, useCallback, useEffect } from 'react';
import { Toaster, toast } from 'sonner';
import {
  ReactFlow,
  Controls,
  Background,
  ConnectionMode,
  useNodesState,
  useEdgesState,
  addEdge,
  useReactFlow,
  ReactFlowProvider,
  MiniMap
} from '@xyflow/react';
import type {
  Node,
  Edge,
  Connection,
  OnSelectionChangeParams,
  NodeChange,
  EdgeChange
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
  runtimeParts?: Record<string, string>;
};

type InferenceNodeData = {
  model: string;
  addSpecial: boolean;
  parseSpecial: boolean;
  maxTokens: number;
  temperature: number;
  topK: number;
  topP: number;
  topPMinKeep: number;
  seed: number;
  grammar: string;
  loraName: string;
  loraPath: string;
  loraScale: number;
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

type GraphConnectionConfig = {
  enabled: boolean;
  path: string;
  autoBuildBackend: boolean;
  autoLoadFrontend: boolean;
  allowFrontendSave: boolean;
};

const MAX_EDGE_TOKENS = 24;

type TokenEvent = {
  nodeId: string;
  tokenChunk: string;
};

const nodePrefix: Record<NodeKind, string> = {
  formatNode: 'format',
  inferenceNode: 'infer',
  outputNode: 'out'
};

const defaultNodeData: Record<NodeKind, FormatNodeData | InferenceNodeData | OutputNodeData> = {
  formatNode: { formatStr: 'User: Hello\nAssistant:' },
  inferenceNode: {
    model: 'qwen2.5-0.5b',
    addSpecial: true,
    parseSpecial: true,
    maxTokens: 150,
    temperature: 0.7,
    topK: 40,
    topP: 0.9,
    topPMinKeep: 1,
    seed: -1,
    grammar: '',
    loraName: '',
    loraPath: '',
    loraScale: 1
  },
  outputNode: { text: '' }
};

const initNodes: AppNode[] = [
  {
    id: 'format_1',
    type: 'formatNode',
    position: { x: 50, y: 150 },
    data: { formatStr: 'User: Hello\nAssistant:' }
  },
  {
    id: 'infer_1',
    type: 'inferenceNode',
    position: { x: 350, y: 130 },
    data: {
      model: 'qwen2.5-0.5b',
      addSpecial: true,
      parseSpecial: true,
      maxTokens: 150,
      temperature: 0.7,
      topK: 40,
      topP: 0.9,
      topPMinKeep: 1,
      seed: -1,
      grammar: '',
      loraName: '',
      loraPath: '',
      loraScale: 1
    }
  },
  {
    id: 'out_1',
    type: 'outputNode',
    position: { x: 700, y: 130 },
    data: { text: '' }
  }
];

const initEdges: AppEdge[] = [
  {
    id: 'e1-2',
    source: 'format_1',
    target: 'infer_1',
    type: 'tokenEdge',
    style: { stroke: '#a855f7', strokeWidth: 2, strokeDasharray: '5,5' },
    animated: false,
    data: { tokens: [] }
  },
  {
    id: 'e2-3',
    source: 'infer_1',
    target: 'out_1',
    type: 'tokenEdge',
    style: { stroke: '#22c55e', strokeWidth: 2, strokeDasharray: '5,5' },
    animated: false,
    data: { tokens: [] }
  }
];

const makeNodeId = (kind: NodeKind, taken: Set<string>, suffix = '') => {
  const base = `${nodePrefix[kind]}_`;
  let i = 1;
  let candidate = `${base}${i}${suffix}`;
  while (taken.has(candidate)) {
    i += 1;
    candidate = `${base}${i}${suffix}`;
  }
  return candidate;
};

const makeEdgeId = (source: string, target: string, taken: Set<string>) => {
  const base = `${source}-${target}`;
  let candidate = base;
  let i = 1;
  while (taken.has(candidate)) {
    candidate = `${base}_${i}`;
    i += 1;
  }
  return candidate;
};

function AppCanvas() {
  const [nodes, setNodes, onNodesChange] = useNodesState<AppNode>(initNodes);
  const [edges, setEdges, onEdgesChange] = useEdgesState<AppEdge>(initEdges);
  const [selectedNodes, setSelectedNodes] = useState<AppNode[]>([]);
  const [selectedEdges, setSelectedEdges] = useState<AppEdge[]>([]);
  const [isRunning, setIsRunning] = useState(false);
  const abortControllerRef = useRef<AbortController | null>(null);
  const [contextMenu, setContextMenu] = useState<{ x: number; y: number; flowX: number; flowY: number } | null>(null);
  const paneRef = useRef<HTMLDivElement | null>(null);
  const { screenToFlowPosition } = useReactFlow<AppNode, AppEdge>();
  const outputBuffers = useRef<Record<string, string>>({});
  const tokenRoutingMode = useRef<'unknown' | 'inferOnly' | 'outputEmits'>('unknown');
  const [graphConfig, setGraphConfig] = useState<GraphConnectionConfig | null>(null);
  const [graphStatus, setGraphStatus] = useState('未读取图连接配置');
  const [isLeftSidebarCollapsed, setIsLeftSidebarCollapsed] = useState(false);
  const [isRightSidebarCollapsed, setIsRightSidebarCollapsed] = useState(false);

  const tokenQueueRef = useRef<TokenEvent[]>([]);
  const flushRafRef = useRef<number | null>(null);
  const nodeByIdRef = useRef<Map<string, AppNode>>(new Map());
  const adjacencyRef = useRef<Map<string, string[]>>(new Map());
  const edgeIdByKeyRef = useRef<Map<string, string>>(new Map());
  const formatRuntimeRef = useRef<Record<string, Record<string, string>>>({});

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

  const selectedNode = selectedNodes.length === 1 && selectedEdges.length === 0 ? selectedNodes[0] : null;
  const selectedEdge = selectedEdges.length === 1 && selectedNodes.length === 0 ? selectedEdges[0] : null;
  const isMultiSelection = selectedNodes.length + selectedEdges.length > 1;

  const flushTokenQueue = useCallback(() => {
    flushRafRef.current = null;

    if (tokenQueueRef.current.length === 0) return;

    const events = tokenQueueRef.current.splice(0, tokenQueueRef.current.length);
    const nodeById = nodeByIdRef.current;
    const adjacency = adjacencyRef.current;
    const edgeIdByKey = edgeIdByKeyRef.current;

    const touchedOutputs = new Set<string>();
    const touchedFormats = new Set<string>();
    const edgeTokenAppends = new Map<string, string[]>();

    for (const { nodeId, tokenChunk } of events) {
      const emittingNode = nodeById.get(nodeId);
      if (!emittingNode) continue;

      let switchedToOutputMode = false;
      if (emittingNode.type === 'outputNode' && tokenRoutingMode.current !== 'outputEmits') {
        tokenRoutingMode.current = 'outputEmits';
        outputBuffers.current = {};
        switchedToOutputMode = true;
      } else if (tokenRoutingMode.current === 'unknown') {
        tokenRoutingMode.current = 'inferOnly';
      }

      const shouldPropagateThroughOutputs =
        tokenRoutingMode.current === 'inferOnly' || emittingNode.type === 'outputNode';

      const nodePathCount = new Map<string, number>();
      const edgePathCount = new Map<string, number>();
      const pending = new Map<string, number>([[nodeId, 1]]);

      let guard = 0;
      while (pending.size > 0 && guard < 10000) {
        guard += 1;
        const [current, delta] = pending.entries().next().value as [string, number];
        pending.delete(current);
        if (delta <= 0) continue;

        const currentNode = nodeById.get(current);
        if (!currentNode) continue;
        nodePathCount.set(current, (nodePathCount.get(current) ?? 0) + delta);

        const nextNodes = adjacency.get(current) ?? [];
        for (const next of nextNodes) {
          const edgeKey = `${current}->${next}`;
          edgePathCount.set(edgeKey, (edgePathCount.get(edgeKey) ?? 0) + delta);

          const nextNode = nodeById.get(next);
          if (!nextNode) continue;
          if (nextNode.type === 'outputNode' && !shouldPropagateThroughOutputs) continue;

          pending.set(next, (pending.get(next) ?? 0) + delta);
        }
      }

      const outputIncrements = new Map<string, number>();
      if (tokenRoutingMode.current === 'inferOnly') {
        if (emittingNode.type === 'inferenceNode') {
          for (const [id, count] of nodePathCount.entries()) {
            const node = nodeById.get(id);
            if (node?.type === 'outputNode' && count > 0) {
              outputIncrements.set(id, (outputIncrements.get(id) ?? 0) + count);
            }
          }
        }
      } else {
        // outputEmits 模式下，后端会直接上报每个 OutputNode 的 token。
        // 这里只更新当前 emitting output，避免 Output->Output 链路被前端再次传播导致翻倍。
        if (emittingNode.type === 'outputNode') {
          outputIncrements.set(nodeId, (outputIncrements.get(nodeId) ?? 0) + 1);
        }
      }

      for (const [targetId, increment] of outputIncrements.entries()) {
        if (increment <= 0 && !switchedToOutputMode) continue;
        const baseText = switchedToOutputMode ? '' : (outputBuffers.current[targetId] ?? '');
        const nextText = increment > 0 ? `${baseText}${tokenChunk.repeat(increment)}` : baseText;
        outputBuffers.current[targetId] = nextText;
        touchedOutputs.add(targetId);
      }

      for (const targetId of adjacency.get(nodeId) ?? []) {
        const targetNode = nodeById.get(targetId);
        if (!targetNode || targetNode.type !== 'formatNode') continue;
        const runtimeParts = formatRuntimeRef.current[targetId] ?? {};
        runtimeParts[nodeId] = `${runtimeParts[nodeId] ?? ''}${tokenChunk}`;
        formatRuntimeRef.current[targetId] = runtimeParts;
        touchedFormats.add(targetId);
      }

      for (const [edgeKey, increment] of edgePathCount.entries()) {
        if (increment <= 0) continue;
        const edgeId = edgeIdByKey.get(edgeKey);
        if (!edgeId) continue;
        const arr = edgeTokenAppends.get(edgeId) ?? [];
        for (let i = 0; i < increment; i += 1) arr.push(tokenChunk);
        edgeTokenAppends.set(edgeId, arr);
      }
    }

    if (touchedOutputs.size > 0 || touchedFormats.size > 0) {
      setNodes((nds) => nds.map((node) => {
        if (node.type === 'outputNode' && touchedOutputs.has(node.id)) {
          const data = node.data as OutputNodeData & { onClear?: (id: string) => void };
          return { ...node, data: { ...data, text: outputBuffers.current[node.id] ?? '' } };
        }
        if (node.type === 'formatNode' && touchedFormats.has(node.id)) {
          const data = node.data as FormatNodeData;
          return { ...node, data: { ...data, runtimeParts: formatRuntimeRef.current[node.id] ?? {} } };
        }
        return node;
      }));
    }

    if (edgeTokenAppends.size > 0) {
      setEdges((eds) => eds.map((edge) => {
        const appended = edgeTokenAppends.get(edge.id);
        if (!appended || appended.length === 0) return edge;
        const prev = edge.data?.tokens ?? [];
        const nextTokens = [...prev, ...appended].slice(-MAX_EDGE_TOKENS);
        return { ...edge, data: { tokens: nextTokens } };
      }));
    }
  }, [setEdges, setNodes]);

  const handleIncomingToken = useCallback((nodeId: string, tokenChunk: string) => {
    tokenQueueRef.current.push({ nodeId, tokenChunk });
    if (flushRafRef.current == null) {
      flushRafRef.current = window.requestAnimationFrame(() => {
        flushTokenQueue();
      });
    }
  }, [flushTokenQueue]);



  const isValidConnection = (connection: Connection | AppEdge) => {
    if (isRunning) return false;
    const source = connection.source ?? null;
    const target = connection.target ?? null;
    if (!source || !target) return false;
    if (source === target) return false;
    return !edges.some((edge) => edge.source === source && edge.target === target);
  };

  const onConnect = (params: Connection) => {
    if (!isValidConnection(params)) return;

    const takenIds = new Set(edges.map((edge) => edge.id));
    const source = params.source as string;
    const target = params.target as string;

    const newEdge: AppEdge = {
      ...params,
      id: makeEdgeId(source, target, takenIds),
      source,
      target,
      type: 'tokenEdge',
      animated: false,
      style: { stroke: '#a855f7', strokeWidth: 2, strokeDasharray: '5,5' },
      data: { tokens: [] }
    };
    setEdges((eds) => addEdge(newEdge, eds));
  };

  const addNodeByContextMenu = (type: NodeKind) => {
    if (!contextMenu || isRunning) return;

    const takenIds = new Set(nodes.map((node) => node.id));
    const nodeId = makeNodeId(type, takenIds);

    const newNode: AppNode = {
      id: nodeId,
      type,
      position: { x: contextMenu.flowX, y: contextMenu.flowY },
      data: structuredClone(defaultNodeData[type])
    } as AppNode;

    setNodes((nds) => [...nds, newNode]);
    setContextMenu(null);
  };


  const clearOutputContent = useCallback((nodeId: string) => {
    if (isRunning) return;
    outputBuffers.current[nodeId] = '';
    setNodes((nds) =>
      nds.map((node) =>
        node.id === nodeId && node.type === 'outputNode'
          ? { ...node, data: { ...(node.data as OutputNodeData), text: '' } }
          : node
      )
    );
  }, [isRunning, setNodes]);

  const deleteNodesAndEdges = (nodeIds: string[], edgeIds: string[]) => {
    if (isRunning) return;

    const nodeSet = new Set(nodeIds);
    const edgeSet = new Set(edgeIds);

    for (const nodeId of nodeSet) {
      delete outputBuffers.current[nodeId];
    }

    setNodes((nds) => nds.filter((node) => !nodeSet.has(node.id)));
    setEdges((eds) =>
      eds.filter((edge) => !edgeSet.has(edge.id) && !nodeSet.has(edge.source) && !nodeSet.has(edge.target))
    );

    setSelectedNodes([]);
    setSelectedEdges([]);
  };

  const deleteNode = (nodeId: string) => deleteNodesAndEdges([nodeId], []);
  const deleteEdge = (edgeId: string) => deleteNodesAndEdges([], [edgeId]);
  const deleteSelection = () => deleteNodesAndEdges(selectedNodes.map((n) => n.id), selectedEdges.map((e) => e.id));

  const saveNode = (nodeId: string, nextData: Record<string, string | number | boolean>) => {
    if (isRunning) return;

    const currentNode = nodes.find((node) => node.id === nodeId);
    if (!currentNode) return;

    const hasChanges = JSON.stringify(currentNode.data) !== JSON.stringify(nextData);
    if (!hasChanges) return;

    const takenIds = new Set(nodes.map((node) => node.id));
    takenIds.delete(nodeId);
    const newNodeId = makeNodeId(currentNode.type as NodeKind, takenIds, '_edit');

    const recreatedNode = {
      ...currentNode,
      id: newNodeId,
      data: nextData,
      selected: true
    } as AppNode;

    if (currentNode.type === 'outputNode') {
      outputBuffers.current[newNodeId] = outputBuffers.current[nodeId] ?? '';
      delete outputBuffers.current[nodeId];
    }

    setNodes((nds) => nds.map((node) => (node.id === nodeId ? recreatedNode : { ...node, selected: false })));
    setEdges((eds) =>
      eds.map((edge) => {
        const source = edge.source === nodeId ? newNodeId : edge.source;
        const target = edge.target === nodeId ? newNodeId : edge.target;
        return { ...edge, source, target, id: `${source}-${target}` };
      })
    );
    setSelectedNodes([recreatedNode]);
    setSelectedEdges([]);
  };

  const saveEdge = (edgeId: string, stroke: string) => {
    if (isRunning) return;
    setEdges((eds) =>
      eds.map((edge) =>
        edge.id === edgeId
          ? { ...edge, style: { ...(edge.style ?? {}), stroke, strokeWidth: 2, strokeDasharray: '5,5' } }
          : edge
      )
    );
  };

  const copySingleNode = (nodeId: string) => {
    if (isRunning) return;
    const currentNode = nodes.find((node) => node.id === nodeId);
    if (!currentNode) return;

    const takenIds = new Set(nodes.map((node) => node.id));
    const copyId = makeNodeId(currentNode.type as NodeKind, takenIds, '_copy');

    const copiedNode = {
      ...currentNode,
      id: copyId,
      selected: true,
      position: { x: currentNode.position.x + 40, y: currentNode.position.y + 40 },
      data: currentNode.type === 'outputNode'
        ? { text: '' }
        : currentNode.data
    } as AppNode;

    setNodes((nds) => [...nds.map((node) => ({ ...node, selected: false })), copiedNode]);
    setSelectedNodes([copiedNode]);
    setSelectedEdges([]);
  };

  const copySelection = () => {
    if (isRunning || selectedNodes.length === 0) return;

    const takenNodeIds = new Set(nodes.map((node) => node.id));
    const idMap = new Map<string, string>();

    const copiedNodes: AppNode[] = selectedNodes.map((node) => {
      const newId = makeNodeId(node.type as NodeKind, takenNodeIds, '_copy');
      takenNodeIds.add(newId);
      idMap.set(node.id, newId);
      return {
        ...node,
        id: newId,
        selected: true,
        position: { x: node.position.x + 40, y: node.position.y + 40 },
        data: node.type === 'outputNode'
          ? { text: '' }
          : node.data
      } as AppNode;
    });

    const selectedEdgeSet = new Set(selectedEdges.map((edge) => edge.id));
    const nodeIdSet = new Set(selectedNodes.map((node) => node.id));
    const takenEdgeIds = new Set(edges.map((edge) => edge.id));

    const copiedEdges: AppEdge[] = edges
      .filter(
        (edge) =>
          selectedEdgeSet.has(edge.id) &&
          nodeIdSet.has(edge.source) &&
          nodeIdSet.has(edge.target)
      )
      .map((edge) => {
        const newSource = idMap.get(edge.source) as string;
        const newTarget = idMap.get(edge.target) as string;
        const newId = makeEdgeId(newSource, newTarget, takenEdgeIds);
        takenEdgeIds.add(newId);

        return {
          ...edge,
          id: newId,
          source: newSource,
          target: newTarget,
          selected: true,
          animated: false,
          style: { ...(edge.style ?? {}), strokeDasharray: '5,5' },
          data: { tokens: [] }
        };
      });

    setNodes((nds) => [...nds.map((node) => ({ ...node, selected: false })), ...copiedNodes]);
    setEdges((eds) => [...eds.map((edge) => ({ ...edge, selected: false })), ...copiedEdges]);
    setSelectedNodes(copiedNodes);
    setSelectedEdges(copiedEdges);
  };

  const handleSelectionChange = (params: OnSelectionChangeParams) => {
    setSelectedNodes((params.nodes ?? []) as AppNode[]);
    setSelectedEdges((params.edges ?? []) as AppEdge[]);
  };

  const handleNodesChange = (changes: NodeChange<AppNode>[]) => {
    onNodesChange(changes);
  };

  const handleEdgesChange = (changes: EdgeChange<AppEdge>[]) => {
    onEdgesChange(changes);
  };

  useEffect(() => {
    nodeByIdRef.current = new Map(nodes.map((node) => [node.id, node]));

    const adjacencySet = new Map<string, Set<string>>();
    const edgeIdByKey = new Map<string, string>();
    for (const edge of edges) {
      if (!adjacencySet.has(edge.source)) adjacencySet.set(edge.source, new Set());
      adjacencySet.get(edge.source)?.add(edge.target);
      edgeIdByKey.set(`${edge.source}->${edge.target}`, edge.id);
    }

    adjacencyRef.current = new Map(
      Array.from(adjacencySet.entries()).map(([source, targets]) => [source, Array.from(targets)])
    );
    edgeIdByKeyRef.current = edgeIdByKey;
  }, [edges, nodes]);

  const normalizeGraphNode = (node: Partial<AppNode> & { type?: string; id?: string }): AppNode | null => {
    if (!node?.id || !node?.type || !['formatNode', 'inferenceNode', 'outputNode'].includes(node.type)) {
      return null;
    }

    const kind = node.type as NodeKind;

    return {
      id: node.id,
      type: kind,
      position: node.position ?? { x: 80, y: 80 },
      data: { ...(defaultNodeData[kind] as Record<string, unknown>), ...((node.data as Record<string, unknown>) ?? {}) }
    } as AppNode;
  };

  const normalizeGraphEdge = (edge: Partial<AppEdge>): AppEdge | null => {
    if (!edge?.source || !edge?.target) return null;
    return {
      id: `${edge.source}-${edge.target}`,
      source: edge.source,
      target: edge.target,
      type: 'tokenEdge',
      style: { stroke: '#a855f7', strokeWidth: 2, strokeDasharray: '5,5' },
      animated: false,
      data: { tokens: [] }
    };
  };

  const applyGraph = useCallback((payload: { nodes?: Partial<AppNode>[]; edges?: Partial<AppEdge>[] }) => {
    const normalizedNodes = (payload.nodes ?? []).map(normalizeGraphNode).filter(Boolean) as AppNode[];
    const normalizedEdges = (payload.edges ?? []).map(normalizeGraphEdge).filter(Boolean) as AppEdge[];
    if (normalizedNodes.length === 0) {
      throw new Error('图配置中没有可用节点');
    }
    setNodes(normalizedNodes);
    setEdges(normalizedEdges);
  }, [setEdges, setNodes]);

  const loadGraphConfig = useCallback(async () => {
    try {
      const response = await fetch('http://localhost:3000/api/graph-config');
      const data = await response.json();
      if (!response.ok) throw new Error(data?.error || '读取图连接配置失败');

      setGraphConfig(data.config ?? null);
      applyGraph(data.graph ?? {});
      setGraphStatus(`已读取图配置：${data.config?.path || ''}`);
    } catch (error) {
      setGraphStatus(error instanceof Error ? error.message : '读取图连接配置失败');
    }
  }, [applyGraph]);

  const saveGraphConfig = useCallback(async () => {
    try {
      const response = await fetch('http://localhost:3000/api/graph-config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          nodes: nodes.map((node) => ({ id: node.id, type: node.type, position: node.position, data: node.data })),
          edges: edges.map((edge) => ({ source: edge.source, target: edge.target }))
        })
      });

      const data = await response.json();
      if (!response.ok) throw new Error(data?.error || '保存图连接配置失败');
      setGraphStatus(`图连接配置已保存至：${data.path || ''}`);
    } catch (error) {
      setGraphStatus(error instanceof Error ? error.message : '保存图连接配置失败');
    }
  }, [edges, nodes]);

  const refreshGraphConfig = useCallback(async () => {
    try {
      const response = await fetch('http://localhost:3000/api/graph-config');
      const data = await response.json();
      if (!response.ok) throw new Error(data?.error || '读取图连接配置失败');
      setGraphConfig(data.config ?? null);
      setGraphStatus(`图连接配置可用：${data.config?.path || ''}`);
    } catch (error) {
      setGraphConfig(null);
      setGraphStatus(error instanceof Error ? error.message : '读取图连接配置失败');
    }
  }, []);

  useEffect(() => {
    void refreshGraphConfig();
  }, [refreshGraphConfig]);

  useEffect(() => {
    if (graphConfig?.autoLoadFrontend) {
      void loadGraphConfig();
    }
  }, [graphConfig?.autoLoadFrontend, loadGraphConfig]);

  useEffect(() => () => {
    if (flushRafRef.current != null) {
      window.cancelAnimationFrame(flushRafRef.current);
      flushRafRef.current = null;
    }
  }, []);

  const runPipeline = async () => {
    if (isRunning) return;
    setIsRunning(true);
    abortControllerRef.current = new AbortController();

    outputBuffers.current = {};
    tokenRoutingMode.current = 'unknown';
    tokenQueueRef.current = [];
    if (flushRafRef.current != null) {
      window.cancelAnimationFrame(flushRafRef.current);
      flushRafRef.current = null;
    }
    formatRuntimeRef.current = Object.fromEntries(
      nodes
        .filter((node) => node.type === 'formatNode')
        .map((node) => [node.id, {} as Record<string, string>])
    );
    setNodes((nds) => nds.map((n) => {
      if (n.type === 'outputNode') return { ...n, data: { ...(n.data as OutputNodeData), text: '' } };
      if (n.type === 'formatNode') return { ...n, data: { ...(n.data as FormatNodeData), runtimeParts: {} } };
      return n;
    }));
    setEdges((eds) => eds.map((e) => ({ ...e, animated: true, style: { ...(e.style ?? {}), strokeDasharray: '5,5' }, data: { tokens: [] } })));

    try {
      const response = await fetch('http://localhost:3000/api/run', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          nodes: nodes.map((node) => ({ id: node.id, type: node.type, data: node.data })),
          edges: Array.from(
            new Map(edges.map((edge) => [`${edge.source}->${edge.target}`, { source: edge.source, target: edge.target }])).values()
          )
        }),
        signal: abortControllerRef.current.signal
      });

      if (!response.body) throw new Error('ReadableStream not supported');

      const reader = response.body.getReader();
      const decoder = new TextDecoder('utf-8');
      let chunkBuffer = '';

      while (true) {
        const { done, value } = await reader.read();
        if (done) break;

        chunkBuffer += decoder.decode(value, { stream: true });
        const lines = chunkBuffer.split('\n');
        chunkBuffer = lines.pop() || '';

        for (const line of lines) {
          if (!line.trim()) continue;

          let data;
          try {
            data = JSON.parse(line);
          } catch {
            console.warn('Skipping invalid JSON chunk:', line);
            continue;
          }

          if (data.error) {
            console.error('Backend Error:', data.error);
            toast.error(`Engine Error: ${data.error}`);
          } else if (data.done) {
            toast.success('Pipeline execution finished!');
          } else if (data.text) {
            handleIncomingToken(data.nodeId, data.text);
          }
        }
      }
    } catch (err: unknown) {
      if (err instanceof Error && err.name === 'AbortError') {
        console.log('Pipeline stopped by user');
      } else {
        console.error('Run pipeline failed:', err);
      }
    } finally {
      flushTokenQueue();
      setIsRunning(false);
      setEdges((eds) => eds.map((e) => ({ ...e, animated: false, style: { ...(e.style ?? {}), strokeDasharray: '5,5' } })));
    }
  };


  const renderNodes = useMemo(
    () =>
      nodes.map((node) =>
        node.type === 'outputNode'
          ? {
            ...node,
            data: {
              ...(node.data as OutputNodeData),
              onClear: clearOutputContent
            }
          }
          : node
      ) as AppNode[],
    [nodes, clearOutputContent]
  );

  const stopPipeline = () => {
    if (abortControllerRef.current) {
      abortControllerRef.current.abort();
    }
  };

  return (
    <div className="relative bg-gray-900 overflow-hidden" style={{ width: '100vw', height: '100vh' }}>
      <Toaster theme="dark" position="top-center" richColors />
      <div className="relative bg-gray-900 overflow-hidden" style={{ width: '100vw', height: '100vh' }}>
        <div className={`absolute left-0 top-0 z-20 h-full transition-transform duration-300 ${isLeftSidebarCollapsed ? '-translate-x-[calc(100%-2rem)]' : 'translate-x-0'}`}>
          <Sidebar
            allEdges={edges}
            selectedNode={selectedNode}
            selectedEdge={selectedEdge}
            selectedNodeCount={selectedNodes.length}
            selectedEdgeCount={selectedEdges.length}
            isMultiSelection={isMultiSelection}
            onDeleteNode={deleteNode}
            onDeleteEdge={deleteEdge}
            onDeleteSelection={deleteSelection}
            onSaveNode={saveNode}
            onSaveEdge={saveEdge}
            onCopyNode={copySingleNode}
            onCopySelection={copySelection}
            onRun={runPipeline}
            onStop={stopPipeline}
            isRunning={isRunning}
            graphConfig={graphConfig}
            graphStatus={graphStatus}
            onLoadGraphConfig={loadGraphConfig}
            onSaveGraphConfig={saveGraphConfig}
          />
          <button
            type="button"
            onClick={() => setIsLeftSidebarCollapsed((prev) => !prev)}
            className="absolute right-0 top-1/2 -translate-y-1/2 translate-x-full w-8 h-20 rounded-r-xl bg-gray-800 border border-gray-700 border-l-0 text-gray-200 hover:bg-gray-700"
            title={isLeftSidebarCollapsed ? '展开左侧栏' : '收起左侧栏'}
          >
            {isLeftSidebarCollapsed ? '>' : '<'}
          </button>
        </div>

        <div ref={paneRef} className="h-full relative" onClick={() => setContextMenu(null)}>
          <ReactFlow
            nodes={renderNodes}
            edges={edges}
            onNodesChange={handleNodesChange}
            onEdgesChange={handleEdgesChange}
            onConnect={onConnect}
            isValidConnection={isValidConnection}
            nodesConnectable={!isRunning}
            connectionMode={ConnectionMode.Strict}
            deleteKeyCode={isRunning ? null : ['Delete', 'Backspace']}
            multiSelectionKeyCode="Shift"
            nodeTypes={nodeTypes}
            edgeTypes={edgeTypes}
            onSelectionChange={handleSelectionChange}
            onlyRenderVisibleElements
            onPaneContextMenu={(event) => {
              event.preventDefault();
              if (isRunning || !paneRef.current) return;
              const rect = paneRef.current.getBoundingClientRect();
              const flowPosition = screenToFlowPosition({ x: event.clientX, y: event.clientY });
              setContextMenu({
                x: event.clientX - rect.left,
                y: event.clientY - rect.top,
                flowX: flowPosition.x,
                flowY: flowPosition.y
              });
            }}
            fitView
            proOptions={{ hideAttribution: true }}
            snapToGrid={true}
            snapGrid={[15, 15]}
          >
            <Background color="#4b5563" gap={15} size={1} />
            <Controls className="bg-gray-800 border-gray-700 fill-gray-300" />
            <MiniMap
              className="bg-gray-900 border border-gray-700 rounded-lg overflow-hidden"
              maskColor="rgba(17, 24, 39, 0.7)"
              nodeColor={(node) => {
                if (node.type === 'inferenceNode') return '#a855f7';
                if (node.type === 'formatNode') return '#3b82f6';
                if (node.type === 'outputNode') return '#22c55e';
                return '#6b7280';
              }}
            />
          </ReactFlow>

          {!isRunning && (
            <div className="absolute bottom-4 left-4 text-xs text-gray-400 bg-gray-900/80 border border-gray-700 rounded px-3 py-2">
              左键从节点右侧小圆点拖到目标左侧小圆点连边；按 Shift 可多选，按 Delete 可删节点/边。
            </div>
          )}

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

        <div className={`absolute right-0 top-0 z-20 h-full transition-transform duration-300 ${isRightSidebarCollapsed ? 'translate-x-[calc(100%-2rem)]' : 'translate-x-0'}`}>
          <button
            type="button"
            onClick={() => setIsRightSidebarCollapsed((prev) => !prev)}
            className="absolute left-0 top-1/2 -translate-y-1/2 -translate-x-full w-8 h-20 rounded-l-xl bg-gray-800 border border-gray-700 border-r-0 text-gray-200 hover:bg-gray-700"
            title={isRightSidebarCollapsed ? '展开日志栏' : '收起日志栏'}
          >
            {isRightSidebarCollapsed ? '<' : '>'}
          </button>
          <LogSidebar />
        </div>
      </div>
    </div>
  );
}


export default function App() {
  return (
    <ReactFlowProvider>
      <AppCanvas />
    </ReactFlowProvider>
  );
}

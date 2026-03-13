import { create } from 'zustand';
import type {
  NodeChange,
  EdgeChange,
  Connection,
} from '@xyflow/react';
import {
  applyNodeChanges,
  applyEdgeChanges,
  addEdge,
} from '@xyflow/react';
import type { AppNode, AppEdge } from '../App';

interface FlowState {
  nodes: AppNode[];
  edges: AppEdge[];
  isRunning: boolean;
  
  // 兼容 React 的 setState 函数式更新: setNodes((prev) => newNodes)
  setNodes: (updater: AppNode[] | ((prev: AppNode[]) => AppNode[])) => void;
  setEdges: (updater: AppEdge[] | ((prev: AppEdge[]) => AppEdge[])) => void;
  setIsRunning: (isRunning: boolean) => void;

  // React Flow 必需的事件处理器
  onNodesChange: (changes: NodeChange[]) => void;
  onEdgesChange: (changes: EdgeChange[]) => void;
}

export const useFlowStore = create<FlowState>((set, get) => ({
  nodes:[
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
],
  edges:[
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
],
  isRunning: false,

  setNodes: (updater) => 
    set((state) => ({
      nodes: typeof updater === 'function' ? updater(state.nodes) : updater,
    })),

  setEdges: (updater) => 
    set((state) => ({
      edges: typeof updater === 'function' ? updater(state.edges) : updater,
    })),

  setIsRunning: (isRunning) => set({ isRunning }),

  onNodesChange: (changes) => {
    set({ nodes: applyNodeChanges(changes, get().nodes) as AppNode[] });
  },
  
  onEdgesChange: (changes) => {
    set({ edges: applyEdgeChanges(changes, get().edges) as AppEdge[] });
  },
}));
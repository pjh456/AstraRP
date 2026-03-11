import { Handle, Position } from '@xyflow/react';
import { Cpu } from 'lucide-react';

export default function InferenceNode({ data }: any) {
  return (
    <div className="bg-gray-800 border border-purple-500 rounded-lg shadow-lg min-w-[200px] overflow-hidden">
      {/* 顶部标题栏 */}
      <div className="bg-purple-600/20 px-3 py-2 border-b border-purple-500/50 flex items-center gap-2">
        <Cpu size={16} className="text-purple-400" />
        <span className="text-sm font-bold text-gray-100">Inference</span>
      </div>

      {/* 节点主体内容 */}
      <div className="p-3 text-xs text-gray-400 flex flex-col gap-1">
        <div className="flex justify-between">
          <span>Model:</span>
          <span className="text-gray-200">{data.model || 'default'}</span>
        </div>
        <div className="flex justify-between">
          <span>Max Tokens:</span>
          <span className="text-gray-200">{data.maxTokens || 100}</span>
        </div>
        <div className="flex justify-between">
          <span>Temp:</span>
          <span className="text-gray-200">{data.temperature || 0.7}</span>
        </div>
      </div>

      {/* 左侧输入端点 (Target) */}
      <Handle type="target" position={Position.Left} className="w-3 h-3 bg-purple-400 border-2 border-gray-800" />
      {/* 右侧输出端点 (Source) */}
      <Handle type="source" position={Position.Right} className="w-3 h-3 bg-purple-400 border-2 border-gray-800" />
    </div>
  );
}
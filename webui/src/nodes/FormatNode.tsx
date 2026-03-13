import { Handle, Position } from '@xyflow/react';
import type { NodeProps } from '@xyflow/react';
import { FileText } from 'lucide-react';

type FormatNodeData = {
  formatStr: string;
  runtimeParts?: Record<string, string>;
};

const parseFormatStr = (formatStr: string) => {
  const regex = /\{\{\s*node:([A-Za-z0-9_-]+)\s*\}\}/g;
  const parts: Array<{ type: 'text' | 'node'; value: string }> = [];
  let last = 0;
  let match: RegExpExecArray | null;

  while ((match = regex.exec(formatStr)) !== null) {
    if (match.index > last) {
      parts.push({ type: 'text', value: formatStr.slice(last, match.index) });
    }
    parts.push({ type: 'node', value: match[1] });
    last = regex.lastIndex;
  }

  if (last < formatStr.length) {
    parts.push({ type: 'text', value: formatStr.slice(last) });
  }

  if (parts.length === 0) parts.push({ type: 'text', value: formatStr });
  return parts;
};

export default function FormatNode({ data }: NodeProps) {
  const nodeData = data as FormatNodeData;
  const parts = parseFormatStr(nodeData.formatStr || '');

  return (
    <div className="bg-gray-800 border border-blue-500 rounded-lg shadow-lg w-[320px] max-w-[320px] overflow-hidden">
      <div className="bg-blue-600/20 px-3 py-2 border-b border-blue-500/50 flex items-center gap-2">
        <FileText size={16} className="text-blue-400" />
        <span className="text-sm font-bold text-gray-100">Format Prompt</span>
      </div>

      {/* 移除了 flex flex-wrap，保留 whitespace-pre-wrap 使 \n 正确断行，并添加 break-words 防溢出 */}
      <div className="p-3 text-xs text-gray-300 whitespace-pre-wrap leading-relaxed max-h-[220px] overflow-y-auto break-words">
        {parts.map((part, index) =>
          part.type === 'text' ? (
            <span key={`${part.type}-${index}`} className="text-gray-200">
              {part.value}
            </span>
          ) : (
            <span
              key={`${part.type}-${index}`}
              className="inline-block align-middle mx-0.5 rounded px-1.5 py-0.5 bg-blue-500/20 border border-blue-400/60 text-blue-200 text-[10px]"
            >
              {nodeData.runtimeParts?.[part.value] || part.value}
            </span>
          )
        )}
      </div>

      <Handle type="target" position={Position.Left} className="w-3 h-3 bg-blue-400 border-2 border-gray-800" />
      <Handle type="source" position={Position.Right} className="w-3 h-3 bg-blue-400 border-2 border-gray-800" />
    </div>
  );
}

import { Handle, Position } from '@xyflow/react';
import type { NodeProps } from '@xyflow/react';
import { Terminal, Trash2 } from 'lucide-react';

type OutputNodeData = {
    text: string;
    onClear?: (id: string) => void;
};

export default function OutputNode({ data, id }: NodeProps) {
    const nodeData = data as OutputNodeData;

    return (
        <div className="bg-gray-800 border border-green-500 rounded-lg shadow-lg w-[320px] max-w-[320px] overflow-hidden flex flex-col">
            <div className="bg-green-600/20 px-3 py-2 border-b border-green-500/50 flex items-center justify-between">
                <div className="flex items-center gap-2">
                    <Terminal size={16} className="text-green-400" />
                    <span className="text-sm font-bold text-gray-100">Output Log</span>
                </div>

                <button
                    onClick={() => nodeData.onClear && nodeData.onClear(id)}
                    className="text-gray-400 hover:text-red-400 transition-colors cursor-pointer"
                    title="Clear content"
                >
                    <Trash2 size={14} />
                </button>
            </div>

            <div className="p-3 text-xs text-gray-300 min-h-[80px] max-h-[220px] overflow-y-auto whitespace-pre-wrap font-mono leading-relaxed break-words">
                {nodeData.text ? (
                    nodeData.text
                ) : (
                    <span className="text-gray-600 italic">Waiting for pipeline...</span>
                )}
            </div>

            <Handle type="target" position={Position.Left} className="w-3 h-3 bg-green-400 border-2 border-gray-800" />
            <Handle type="source" position={Position.Right} className="w-3 h-3 bg-green-400 border-2 border-gray-800" />
        </div>
    );
}

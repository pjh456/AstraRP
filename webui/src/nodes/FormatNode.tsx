import { Handle, Position } from '@xyflow/react';
import { FileText } from 'lucide-react';

export default function FormatNode({ data }: any) {
    return (
        <div className="bg-gray-800 border border-blue-500 rounded-lg shadow-lg min-w-[200px] overflow-hidden">
            <div className="bg-blue-600/20 px-3 py-2 border-b border-blue-500/50 flex items-center gap-2">
                <FileText size={16} className="text-blue-400" />
                <span className="text-sm font-bold text-gray-100">Format Prompt</span>
            </div>

            <div className="p-3 text-xs text-gray-400">
                <p className="truncate w-40 text-gray-200" title={data.formatStr}>
                    {data.formatStr || 'User: {input}\nAssistant:'}
                </p>
            </div>

            <Handle type="target" position={Position.Left} className="w-3 h-3 bg-blue-400 border-2 border-gray-800" />
            <Handle type="source" position={Position.Right} className="w-3 h-3 bg-blue-400 border-2 border-gray-800" />
        </div>
    );
}
import { Handle, Position } from '@xyflow/react';
import { Terminal, Trash2 } from 'lucide-react';

export default function OutputNode({ data, id }: any) {
    return (
        <div className="bg-gray-800 border border-green-500 rounded-lg shadow-lg min-w-[250px] max-w-[350px] overflow-hidden flex flex-col">
            {/* 顶部标题栏 */}
            <div className="bg-green-600/20 px-3 py-2 border-b border-green-500/50 flex items-center justify-between">
                <div className="flex items-center gap-2">
                    <Terminal size={16} className="text-green-400" />
                    <span className="text-sm font-bold text-gray-100">Output Log</span>
                </div>

                {/* 清空按钮：触发绑定的 onClear 回调 */}
                <button
                    onClick={() => data.onClear && data.onClear(id)}
                    className="text-gray-400 hover:text-red-400 transition-colors cursor-pointer"
                    title="Clear content"
                >
                    <Trash2 size={14} />
                </button>
            </div>

            {/* 内容显示区：支持流式打字机效果的换行显示 */}
            <div className="p-3 text-xs text-gray-300 min-h-[80px] max-h-[200px] overflow-y-auto whitespace-pre-wrap font-mono leading-relaxed">
                {data.text ? (
                    data.text
                ) : (
                    <span className="text-gray-600 italic">Waiting for pipeline...</span>
                )}
            </div>

            {/* 既可以做接收端，也可以做转发源头 */}
            <Handle type="target" position={Position.Left} className="w-3 h-3 bg-green-400 border-2 border-gray-800" />
            <Handle type="source" position={Position.Right} className="w-3 h-3 bg-green-400 border-2 border-gray-800" />
        </div>
    );
}
import type { Node } from '@xyflow/react';
import { useState } from 'react';

interface SidebarProps {
    selectedNode: Node | null;
    onDeleteNode: (nodeId: string) => void;
    onSaveNode: (nodeId: string, nextData: Record<string, string | number>) => void;
    onRun: () => void;
    onStop: () => void;
    isRunning: boolean;
}

function NodePropertyEditor({ selectedNode, onDeleteNode, onSaveNode, isRunning }: Pick<SidebarProps, 'selectedNode' | 'onDeleteNode' | 'onSaveNode' | 'isRunning'>) {
    const [draftData, setDraftData] = useState<Record<string, string | number>>((selectedNode?.data as Record<string, string | number>) || {});

    const handleInputChange = (key: string, value: string) => {
        const originalValue = selectedNode?.data?.[key];
        const parsedValue = typeof originalValue === 'number' ? Number(value) : value;
        setDraftData((prev) => ({ ...prev, [key]: parsedValue }));
    };

    const hasChanges = selectedNode
        ? JSON.stringify(selectedNode.data) !== JSON.stringify(draftData)
        : false;

    if (!selectedNode) {
        return (
            <div className="h-full flex items-center justify-center text-sm text-gray-500">
                Select a node to edit properties
            </div>
        );
    }

    return (
        <div className="space-y-4">
            <h2 className="text-sm font-semibold text-gray-300 uppercase tracking-wider">
                {selectedNode.type} Properties
            </h2>

            <div className="bg-gray-800 p-3 rounded border border-gray-700 space-y-3">
                <div className="flex flex-col gap-1">
                    <label className="text-xs text-gray-400">Node ID</label>
                    <input disabled className="bg-gray-900 border border-gray-700 rounded px-2 py-1 text-sm text-gray-300" value={selectedNode.id} />
                </div>

                {Object.entries(draftData).map(([key, value]) => (
                    <div key={key} className="flex flex-col gap-1">
                        <label className="text-xs text-gray-400 capitalize">{key}</label>
                        <input
                            disabled={isRunning}
                            className="bg-gray-900 border border-gray-700 rounded px-2 py-1 text-sm text-gray-300 focus:border-purple-500 focus:outline-none"
                            value={String(value ?? '')}
                            onChange={(event) => handleInputChange(key, event.target.value)}
                        />
                    </div>
                ))}

                <div className="flex gap-2 pt-2">
                    <button
                        onClick={() => onDeleteNode(selectedNode.id)}
                        disabled={isRunning}
                        className={`flex-1 py-2 rounded text-sm font-medium ${isRunning ? 'bg-gray-700 text-gray-500 cursor-not-allowed' : 'bg-red-600 hover:bg-red-700 text-white'}`}
                    >
                        删除节点
                    </button>
                    <button
                        onClick={() => onSaveNode(selectedNode.id, draftData)}
                        disabled={isRunning || !hasChanges}
                        className={`flex-1 py-2 rounded text-sm font-medium ${(isRunning || !hasChanges) ? 'bg-gray-700 text-gray-500 cursor-not-allowed' : 'bg-blue-600 hover:bg-blue-700 text-white'}`}
                    >
                        保存属性
                    </button>
                </div>
            </div>
        </div>
    );
}

export default function Sidebar({ selectedNode, onDeleteNode, onSaveNode, onRun, onStop, isRunning }: SidebarProps) {
    return (
        <aside className="w-80 h-full bg-gray-900 border-r border-gray-800 flex flex-col">
            <div className="p-4 border-b border-gray-800">
                <h1 className="text-xl font-bold bg-gradient-to-r from-purple-400 to-blue-400 bg-clip-text text-transparent">
                    Astra RP Studio
                </h1>
                <p className="text-xs text-gray-500 mt-1">LLM Pipeline Editor</p>
            </div>

            <div className="flex-1 p-4 overflow-y-auto">
                <NodePropertyEditor
                    key={selectedNode?.id || 'empty'}
                    selectedNode={selectedNode}
                    onDeleteNode={onDeleteNode}
                    onSaveNode={onSaveNode}
                    isRunning={isRunning}
                />
            </div>

            <div className="p-4 border-t border-gray-800 flex gap-2">
                <button
                    type="button" // 防止触发意外的页面重载
                    onClick={onRun}
                    disabled={isRunning}
                    className={`flex-1 py-2 rounded transition-colors font-medium shadow-lg 
                        ${isRunning
                            ? 'bg-gray-600 text-gray-400 cursor-not-allowed'
                            : 'bg-purple-600 hover:bg-purple-700 text-white shadow-purple-900/20'}`}
                >
                    {isRunning ? 'Running...' : '▶ Run'}
                </button>
                <button
                    type="button" // 防止触发意外的页面重载
                    onClick={onStop}
                    disabled={!isRunning}
                    className={`px-4 py-2 rounded transition-colors font-medium shadow-lg
                        ${!isRunning
                            ? 'bg-gray-800 text-gray-600 cursor-not-allowed'
                            : 'bg-red-600 hover:bg-red-700 text-white shadow-red-900/20'}`}
                >
                    ⏹ Stop
                </button>
            </div>
        </aside>
    );
}

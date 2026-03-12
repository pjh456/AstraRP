import type { Edge, Node } from '@xyflow/react';
import { useState } from 'react';

interface SidebarProps {
    selectedNode: Node | null;
    selectedEdge: Edge | null;
    selectedNodeCount: number;
    selectedEdgeCount: number;
    isMultiSelection: boolean;
    onDeleteNode: (nodeId: string) => void;
    onDeleteEdge: (edgeId: string) => void;
    onDeleteSelection: () => void;
    onSaveNode: (nodeId: string, nextData: Record<string, string | number>) => void;
    onSaveEdge: (edgeId: string, stroke: string) => void;
    onCopyNode: (nodeId: string) => void;
    onCopySelection: () => void;
    onRun: () => void;
    onStop: () => void;
    isRunning: boolean;
}

const toEditableNodeData = (data: Record<string, unknown> | undefined): Record<string, string | number> => {
    if (!data) return {};

    return Object.fromEntries(
        Object.entries(data).filter(([, value]) => typeof value === 'string' || typeof value === 'number')
    ) as Record<string, string | number>;
};

function MultiSelectionEditor({ selectedNodeCount, selectedEdgeCount, onCopySelection, onDeleteSelection, isRunning }: Pick<SidebarProps, 'selectedNodeCount' | 'selectedEdgeCount' | 'onCopySelection' | 'onDeleteSelection' | 'isRunning'>) {
    return (
        <div className="space-y-4">
            <h2 className="text-sm font-semibold text-gray-300 uppercase tracking-wider">Multi Selection</h2>
            <div className="bg-gray-800 p-3 rounded border border-gray-700 space-y-3">
                <p className="text-sm text-gray-300">已选中 {selectedNodeCount} 个节点，{selectedEdgeCount} 条边</p>
                <div className="flex gap-2">
                    <button
                        onClick={onCopySelection}
                        disabled={isRunning || selectedNodeCount === 0}
                        className={`flex-1 py-2 rounded text-sm font-medium ${(isRunning || selectedNodeCount === 0) ? 'bg-gray-700 text-gray-500 cursor-not-allowed' : 'bg-indigo-600 hover:bg-indigo-700 text-white'}`}
                    >
                        复制所选
                    </button>
                    <button
                        onClick={onDeleteSelection}
                        disabled={isRunning}
                        className={`flex-1 py-2 rounded text-sm font-medium ${isRunning ? 'bg-gray-700 text-gray-500 cursor-not-allowed' : 'bg-red-600 hover:bg-red-700 text-white'}`}
                    >
                        删除所选
                    </button>
                </div>
            </div>
        </div>
    );
}

function NodePropertyEditor({ selectedNode, onDeleteNode, onSaveNode, onCopyNode, isRunning }: Pick<SidebarProps, 'selectedNode' | 'onDeleteNode' | 'onSaveNode' | 'onCopyNode' | 'isRunning'>) {
    const editableNodeData = toEditableNodeData(selectedNode?.data as Record<string, unknown> | undefined);
    const [draftData, setDraftData] = useState<Record<string, string | number>>(editableNodeData);

    if (!selectedNode) return null;

    const handleInputChange = (key: string, value: string) => {
        const originalValue = editableNodeData[key];
        const parsedValue = typeof originalValue === 'number' ? Number(value) : value;
        setDraftData((prev) => ({ ...prev, [key]: parsedValue }));
    };

    const hasChanges = JSON.stringify(editableNodeData) !== JSON.stringify(draftData);

    return (
        <div className="space-y-4">
            <h2 className="text-sm font-semibold text-gray-300 uppercase tracking-wider">{selectedNode.type} Properties</h2>

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

                <div className="grid grid-cols-2 gap-2 pt-2">
                    <button
                        onClick={() => onCopyNode(selectedNode.id)}
                        disabled={isRunning}
                        className={`py-2 rounded text-sm font-medium ${isRunning ? 'bg-gray-700 text-gray-500 cursor-not-allowed' : 'bg-indigo-600 hover:bg-indigo-700 text-white'}`}
                    >
                        复制节点
                    </button>
                    <button
                        onClick={() => onDeleteNode(selectedNode.id)}
                        disabled={isRunning}
                        className={`py-2 rounded text-sm font-medium ${isRunning ? 'bg-gray-700 text-gray-500 cursor-not-allowed' : 'bg-red-600 hover:bg-red-700 text-white'}`}
                    >
                        删除节点
                    </button>
                    <button
                        onClick={() => onSaveNode(selectedNode.id, draftData)}
                        disabled={isRunning || !hasChanges}
                        className={`col-span-2 py-2 rounded text-sm font-medium ${(isRunning || !hasChanges) ? 'bg-gray-700 text-gray-500 cursor-not-allowed' : 'bg-blue-600 hover:bg-blue-700 text-white'}`}
                    >
                        保存属性
                    </button>
                </div>
            </div>
        </div>
    );
}

function EdgePropertyEditor({ selectedEdge, onDeleteEdge, onSaveEdge, isRunning }: Pick<SidebarProps, 'selectedEdge' | 'onDeleteEdge' | 'onSaveEdge' | 'isRunning'>) {
    const [stroke, setStroke] = useState((selectedEdge?.style?.stroke as string) || '#a855f7');

    if (!selectedEdge) return null;

    const initialStroke = (selectedEdge.style?.stroke as string) || '#a855f7';
    const hasChanges = initialStroke !== stroke;

    return (
        <div className="space-y-4">
            <h2 className="text-sm font-semibold text-gray-300 uppercase tracking-wider">Edge Properties</h2>
            <div className="bg-gray-800 p-3 rounded border border-gray-700 space-y-3">
                <div className="flex flex-col gap-1">
                    <label className="text-xs text-gray-400">Edge ID</label>
                    <input disabled className="bg-gray-900 border border-gray-700 rounded px-2 py-1 text-sm text-gray-300" value={selectedEdge.id} />
                </div>

                <div className="flex flex-col gap-1">
                    <label className="text-xs text-gray-400">颜色</label>
                    <input
                        type="color"
                        disabled={isRunning}
                        className="h-9 bg-gray-900 border border-gray-700 rounded"
                        value={stroke}
                        onChange={(event) => setStroke(event.target.value)}
                    />
                </div>

                <div className="flex gap-2 pt-2">
                    <button
                        onClick={() => onDeleteEdge(selectedEdge.id)}
                        disabled={isRunning}
                        className={`flex-1 py-2 rounded text-sm font-medium ${isRunning ? 'bg-gray-700 text-gray-500 cursor-not-allowed' : 'bg-red-600 hover:bg-red-700 text-white'}`}
                    >
                        删除边
                    </button>
                    <button
                        onClick={() => onSaveEdge(selectedEdge.id, stroke)}
                        disabled={isRunning || !hasChanges}
                        className={`flex-1 py-2 rounded text-sm font-medium ${(isRunning || !hasChanges) ? 'bg-gray-700 text-gray-500 cursor-not-allowed' : 'bg-blue-600 hover:bg-blue-700 text-white'}`}
                    >
                        保存边属性
                    </button>
                </div>
            </div>
        </div>
    );
}

export default function Sidebar(props: SidebarProps) {
    const {
        selectedNode,
        selectedEdge,
        selectedNodeCount,
        selectedEdgeCount,
        isMultiSelection,
        onDeleteNode,
        onDeleteEdge,
        onDeleteSelection,
        onSaveNode,
        onSaveEdge,
        onCopyNode,
        onCopySelection,
        onRun,
        onStop,
        isRunning
    } = props;

    return (
        <aside className="w-80 h-full bg-gray-900 border-r border-gray-800 flex flex-col">
            <div className="p-4 border-b border-gray-800">
                <h1 className="text-xl font-bold bg-gradient-to-r from-purple-400 to-blue-400 bg-clip-text text-transparent">Astra RP Studio</h1>
                <p className="text-xs text-gray-500 mt-1">LLM Pipeline Editor</p>
            </div>

            <div className="flex-1 p-4 overflow-y-auto">
                {isMultiSelection ? (
                    <MultiSelectionEditor
                        selectedNodeCount={selectedNodeCount}
                        selectedEdgeCount={selectedEdgeCount}
                        onCopySelection={onCopySelection}
                        onDeleteSelection={onDeleteSelection}
                        isRunning={isRunning}
                    />
                ) : selectedNode ? (
                    <NodePropertyEditor
                        key={selectedNode.id}
                        selectedNode={selectedNode}
                        onDeleteNode={onDeleteNode}
                        onSaveNode={onSaveNode}
                        onCopyNode={onCopyNode}
                        isRunning={isRunning}
                    />
                ) : selectedEdge ? (
                    <EdgePropertyEditor
                        key={selectedEdge.id}
                        selectedEdge={selectedEdge}
                        onDeleteEdge={onDeleteEdge}
                        onSaveEdge={onSaveEdge}
                        isRunning={isRunning}
                    />
                ) : (
                    <div className="h-full flex items-center justify-center text-sm text-gray-500">
                        Select node/edge to edit, Shift 多选后可批量复制或删除
                    </div>
                )}
            </div>

            <div className="p-4 border-t border-gray-800 flex gap-2">
                <button
                    type="button"
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
                    type="button"
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

import type { Node } from '@xyflow/react';

interface SidebarProps {
    selectedNode: Node | null;
}

export default function Sidebar({ selectedNode }: SidebarProps) {
    return (
        <aside className="w-80 h-full bg-gray-900 border-r border-gray-800 flex flex-col">
            <div className="p-4 border-b border-gray-800">
                <h1 className="text-xl font-bold bg-gradient-to-r from-purple-400 to-blue-400 bg-clip-text text-transparent">
                    Astra RP Studio
                </h1>
                <p className="text-xs text-gray-500 mt-1">LLM Pipeline Editor</p>
            </div>

            <div className="flex-1 p-4 overflow-y-auto">
                {selectedNode ? (
                    <div className="space-y-4">
                        <h2 className="text-sm font-semibold text-gray-300 uppercase tracking-wider">
                            {selectedNode.type} Properties
                        </h2>

                        <div className="bg-gray-800 p-3 rounded border border-gray-700 space-y-3">
                            <div className="flex flex-col gap-1">
                                <label className="text-xs text-gray-400">Node ID</label>
                                <input disabled className="bg-gray-900 border border-gray-700 rounded px-2 py-1 text-sm text-gray-300" value={selectedNode.id} />
                            </div>

                            {/* 动态显示不同节点的配置项 (此处仅做展示，暂无双向绑定) */}
                            {Object.entries(selectedNode.data).map(([key, value]) => (
                                <div key={key} className="flex flex-col gap-1">
                                    <label className="text-xs text-gray-400 capitalize">{key}</label>
                                    <input
                                        readOnly
                                        className="bg-gray-900 border border-gray-700 rounded px-2 py-1 text-sm text-gray-300 focus:border-purple-500 focus:outline-none"
                                        value={value as string}
                                    />
                                </div>
                            ))}
                        </div>
                    </div>
                ) : (
                    <div className="h-full flex items-center justify-center text-sm text-gray-500">
                        Select a node to edit properties
                    </div>
                )}
            </div>

            <div className="p-4 border-t border-gray-800">
                <button className="w-full py-2 bg-purple-600 hover:bg-purple-700 text-white rounded transition-colors font-medium shadow-lg shadow-purple-900/20">
                    ▶ Run Pipeline
                </button>
            </div>
        </aside>
    );
}
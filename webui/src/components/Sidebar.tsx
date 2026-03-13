import type { Edge, Node } from '@xyflow/react';
import { useState } from 'react';

type EditableValue = string | number | boolean;

interface SidebarProps {
  selectedNode: Node | null;
  selectedEdge: Edge | null;
  selectedNodeCount: number;
  selectedEdgeCount: number;
  isMultiSelection: boolean;
  allEdges: Edge[];
  onDeleteNode: (nodeId: string) => void;
  onDeleteEdge: (edgeId: string) => void;
  onDeleteSelection: () => void;
  onSaveNode: (nodeId: string, nextData: Record<string, EditableValue>) => void;
  onSaveEdge: (edgeId: string, stroke: string) => void;
  onCopyNode: (nodeId: string) => void;
  onCopySelection: () => void;
  onRun: () => void;
  onStop: () => void;
  isRunning: boolean;
}

const toEditableNodeData = (data: Record<string, unknown> | undefined): Record<string, EditableValue> => {
  if (!data) return {};

  return Object.fromEntries(
    Object.entries(data).filter(([, value]) => typeof value === 'string' || typeof value === 'number' || typeof value === 'boolean')
  ) as Record<string, EditableValue>;
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

function NodePropertyEditor({ selectedNode, allEdges, onDeleteNode, onSaveNode, onCopyNode, isRunning }: Pick<SidebarProps, 'selectedNode' | 'allEdges' | 'onDeleteNode' | 'onSaveNode' | 'onCopyNode' | 'isRunning'>) {
  const editableNodeData = toEditableNodeData(selectedNode?.data as Record<string, unknown> | undefined);
  const [draftData, setDraftData] = useState<Record<string, EditableValue>>(editableNodeData);

  if (!selectedNode) return null;

  const handleTextOrNumberChange = (key: string, value: string) => {
    const originalValue = editableNodeData[key];
    const parsedValue = typeof originalValue === 'number' ? Number(value) : value;
    setDraftData((prev) => ({ ...prev, [key]: parsedValue }));
  };

  const handleBooleanChange = (key: string, value: boolean) => {
    setDraftData((prev) => ({ ...prev, [key]: value }));
  };

  const upstreamIds = allEdges.filter((edge) => edge.target === selectedNode.id).map((edge) => edge.source);

  const insertPlaceholder = (sourceId: string) => {
    const current = String(draftData.formatStr ?? '');
    const placeholder = `{{node:${sourceId}}}`;
    setDraftData((prev) => ({ ...prev, formatStr: `${current}${placeholder}` }));
  };

  const hasChanges = JSON.stringify(editableNodeData) !== JSON.stringify(draftData);

  const renderInferenceField = (key: string, label: string) => {
    const value = draftData[key];
    if (typeof value === 'boolean') {
      return (
        <div key={key} className="flex items-center justify-between gap-2">
          <label className="text-xs text-gray-400">{label}</label>
          <input
            type="checkbox"
            disabled={isRunning}
            checked={Boolean(value)}
            onChange={(event) => handleBooleanChange(key, event.target.checked)}
          />
        </div>
      );
    }

    return (
      <div key={key} className="flex flex-col gap-1">
        <label className="text-xs text-gray-400">{label}</label>
        <input
          disabled={isRunning}
          className="bg-gray-900 border border-gray-700 rounded px-2 py-1 text-sm text-gray-300 focus:border-purple-500 focus:outline-none"
          value={String(value ?? '')}
          onChange={(event) => handleTextOrNumberChange(key, event.target.value)}
        />
      </div>
    );
  };

  const inferenceFields: Array<{ key: string; label: string }> = [
    { key: 'model', label: 'Model' },
    { key: 'addSpecial', label: 'Add Special Tokens' },
    { key: 'parseSpecial', label: 'Parse Special Tokens' },
    { key: 'maxTokens', label: 'Max Tokens' },
    { key: 'temperature', label: 'Temperature' },
    { key: 'topK', label: 'Top K' },
    { key: 'topP', label: 'Top P' },
    { key: 'topPMinKeep', label: 'Top P Min Keep' },
    { key: 'seed', label: 'Seed' },
    { key: 'grammar', label: 'Grammar' }
  ];

  return (
    <div className="space-y-4">
      <h2 className="text-sm font-semibold text-gray-300 uppercase tracking-wider">{selectedNode.type} Properties</h2>

      <div className="bg-gray-800 p-3 rounded border border-gray-700 space-y-3">
        <div className="flex flex-col gap-1">
          <label className="text-xs text-gray-400">Node ID</label>
          <input disabled className="bg-gray-900 border border-gray-700 rounded px-2 py-1 text-sm text-gray-300" value={selectedNode.id} />
        </div>

        {selectedNode.type === 'formatNode' && (
          <div className="space-y-2">
            <label className="text-xs text-gray-400">Upstream placeholders</label>
            <div className="flex gap-2 overflow-x-auto pb-1">
              {upstreamIds.length > 0 ? upstreamIds.map((sourceId) => (
                <button
                  key={sourceId}
                  type="button"
                  disabled={isRunning}
                  onClick={() => insertPlaceholder(sourceId)}
                  className="shrink-0 rounded-md px-2 py-1 text-xs border border-blue-500/70 bg-blue-500/20 text-blue-200 hover:bg-blue-500/30"
                >
                  {sourceId}
                </button>
              )) : <span className="text-xs text-gray-500">暂无上游节点</span>}
            </div>
          </div>
        )}

        {selectedNode.type === 'inferenceNode'
          ? inferenceFields.filter(({ key }) => key in draftData).map(({ key, label }) => renderInferenceField(key, label))
          : Object.entries(draftData).map(([key, value]) => (
            <div key={key} className="flex flex-col gap-1">
              <label className="text-xs text-gray-400 capitalize">{key}</label>
              <input
                disabled={isRunning}
                className="bg-gray-900 border border-gray-700 rounded px-2 py-1 text-sm text-gray-300 focus:border-purple-500 focus:outline-none"
                value={String(value ?? '')}
                onChange={(event) => handleTextOrNumberChange(key, event.target.value)}
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
    allEdges,
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
            allEdges={allEdges}
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
          <div className="text-sm text-gray-500">选择一个节点或边进行编辑</div>
        )}
      </div>

      <div className="p-4 border-t border-gray-800 space-y-2">
        {!isRunning ? (
          <button onClick={onRun} className="w-full py-2 rounded bg-purple-600 hover:bg-purple-700 text-white font-medium">
            ▶ Run
          </button>
        ) : (
          <button onClick={onStop} className="w-full py-2 rounded bg-red-600 hover:bg-red-700 text-white font-medium">
            ■ Stop
          </button>
        )}
      </div>
    </aside>
  );
}

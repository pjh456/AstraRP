import { useState, useEffect, useRef } from 'react';
import { Terminal, Trash2 } from 'lucide-react';

type LogLevel = 'DEBUG' | 'INFO' | 'WARN' | 'ERROR' | 'FATAL';

interface LogEntry {
    id: number;
    timestamp: string;
    level: LogLevel;
    file: string;
    line: number;
    msg: string;
}

const LEVEL_COLORS: Record<LogLevel, string> = {
    DEBUG: 'text-gray-500',
    INFO: 'text-blue-400',
    WARN: 'text-yellow-400',
    ERROR: 'text-red-400',
    FATAL: 'text-red-600 font-bold bg-red-900/20',
};

export default function LogSidebar() {
    const [logs, setLogs] = useState<LogEntry[]>([]);
    // 默认多选：只看INFO及以上
    const [activeLevels, setActiveLevels] = useState<Set<LogLevel>>(new Set(['INFO', 'WARN', 'ERROR', 'FATAL']));
    const endRef = useRef<HTMLDivElement>(null);
    const [autoScroll, setAutoScroll] = useState(true);

    useEffect(() => {
        // 利用原生 WebSocket 保持长连接接收日志流
        const ws = new WebSocket('ws://localhost:3000/api/logs');
        let logId = 0;

        ws.onmessage = (e) => {
            const data = JSON.parse(e.data);
            setLogs((prev) => {
                // 最多保留500条日志防止内存泄漏
                const newLogs = [...prev, { ...data, id: logId++ }];
                return newLogs.length > 500 ? newLogs.slice(newLogs.length - 500) : newLogs;
            });
        };

        ws.onerror = (err) => {
            console.error("Log WebSocket Error:", err);
        };

        return () => {
            if (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING) {
                ws.close();
            }
        };
    }, []);

    useEffect(() => {
        if (autoScroll) {
            endRef.current?.scrollIntoView({ behavior: 'smooth' });
        }
    }, [logs, autoScroll]);

    const toggleLevel = (level: LogLevel) => {
        const newLevels = new Set(activeLevels);
        if (newLevels.has(level)) newLevels.delete(level);
        else newLevels.add(level);
        setActiveLevels(newLevels);
    };

    const filteredLogs = logs.filter(log => activeLevels.has(log.level));

    return (
        <aside className="w-80 h-full bg-gray-900 border-l border-gray-800 flex flex-col z-10">
            {/* Header */}
            <div className="p-4 border-b border-gray-800 flex justify-between items-center bg-gray-900">
                <div className="flex items-center gap-2">
                    <Terminal size={18} className="text-gray-400" />
                    <h2 className="text-sm font-bold text-gray-200 uppercase tracking-wider">System Logs</h2>
                </div>
                <button onClick={() => setLogs([])} className="text-gray-500 hover:text-red-400 transition-colors">
                    <Trash2 size={16} />
                </button>
            </div>

            {/* Filters (多选支持) */}
            <div className="p-3 border-b border-gray-800 flex flex-wrap gap-2 bg-gray-800/50">
                {(['DEBUG', 'INFO', 'WARN', 'ERROR', 'FATAL'] as LogLevel[]).map(level => (
                    <button
                        key={level}
                        onClick={() => toggleLevel(level)}
                        className={`px-2 py-1 text-[10px] rounded cursor-pointer font-mono border transition-colors ${activeLevels.has(level)
                            ? 'bg-gray-700 border-gray-500 text-gray-200'
                            : 'bg-gray-800 border-gray-700/50 text-gray-600'
                            }`}
                    >
                        {level}
                    </button>
                ))}
            </div>

            {/* Log Stream */}
            <div
                className="flex-1 overflow-y-auto p-3 space-y-2 text-xs font-mono"
                onWheel={() => setAutoScroll(false)} // 滚轮滑动暂停自动滚动
            >
                {filteredLogs.map(log => (
                    <div key={log.id} className="flex flex-col border-b border-gray-800/50 pb-1">
                        <div className="flex items-center gap-2 mb-1">
                            <span className="text-gray-500 text-[10px]">{log.timestamp}</span>
                            <span className={`text-[10px] px-1 rounded bg-gray-800 ${LEVEL_COLORS[log.level]}`}>
                                {log.level}
                            </span>
                            <span className="text-gray-600 text-[10px] truncate max-w-[120px]" title={`${log.file}:${log.line}`}>
                                {log.file.split(/[/\\]/).pop()}:{log.line}
                            </span>
                        </div>
                        <span className="text-gray-300 break-words leading-relaxed">{log.msg}</span>
                    </div>
                ))}
                <div ref={endRef} />
            </div>

            {/* Re-enable auto scroll prompt */}
            {!autoScroll && (
                <div className="absolute bottom-4 right-1/2 translate-x-1/2">
                    <button
                        onClick={() => setAutoScroll(true)}
                        className="bg-purple-600/80 hover:bg-purple-600 text-white text-xs px-3 py-1 rounded-full shadow-lg"
                    >
                        ↓ Resume Scroll
                    </button>
                </div>
            )}
        </aside>
    );
}
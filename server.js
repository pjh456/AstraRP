const express = require('express');
const cors = require('cors');
const path = require('path');
const http = require('http');
const WebSocket = require('ws');

// 引入 C++ 引擎
const astra = require('./build/Release/astrarp_node.node');

const app = express();
app.use(cors());
app.use(express.json());

// 创建 HTTP 服务器以便挂载 WebSocket
const server = http.createServer(app);
const wss = new WebSocket.Server({ server, path: '/api/logs' });

// 设置全局 WS 客户端集合
const logClients = new Set();

wss.on('connection', (ws) => {
    logClients.add(ws);
    ws.on('close', () => logClients.delete(ws));
});


// 1. 初始化 C++ 引擎全局状态
try {
    const levelMap = { 0: 'DEBUG', 1: 'INFO', 2: 'WARN', 3: 'ERROR', 4: 'FATAL' };
    astra.setLogCallback((levelNum, file, line, msg) => {
        const level = levelMap[levelNum] || 'UNKNOWN';
        const logEntry = JSON.stringify({
            timestamp: new Date().toLocaleTimeString(),
            level, file, line, msg
        });
        // 通过 WebSocket 广播给所有连上的前端客户端
        for (const client of logClients) {
            // 确保连接处于打开状态才发送，避免阻塞报错
            if (client.readyState === WebSocket.OPEN) {
                client.send(logEntry);
            }
        }
    });

    const configPath = path.resolve("./config.json");
    console.log("Initializing Astra Engine with:", configPath);
    astra.initSystem(configPath);
    console.log("Astra Engine initialized successfully.");
} catch (err) {
    console.error("Failed to init system:", err);
    process.exit(1);
}

// 2. 提供流式推理 API
app.post('/api/run', (req, res) => {
    const { formatStr } = req.body;

    // 设置 HTTP 响应头，保持连接不断开，用于流式传输
    res.setHeader('Content-Type', 'text/plain; charset=utf-8');
    res.setHeader('Transfer-Encoding', 'chunked');

    console.log("Building pipeline for new request...");
    const pipeline = new astra.Pipeline();

    // 根据前端传来的参数构建 DAG (暂时硬编码图结构，参数动态化)
    pipeline.addFormatNode("format_1", formatStr || "User: Hello\nAssistant:");
    pipeline.addInferenceNode("infer_1");
    pipeline.addOutputNode("out_1");

    pipeline.addEdge("format_1", "infer_1");
    pipeline.addEdge("infer_1", "out_1");

    // 绑定 Token 流水线回调
    pipeline.onToken((nodeId, text) => {
        // 每生成一个 Token，就立刻转成 JSON 字符串并加一个换行符推给前端
        const chunk = JSON.stringify({ nodeId, text }) + '\n';
        res.write(chunk);
    });

    // 开始执行
    pipeline.run((err, result) => {
        if (err) {
            console.error("Pipeline Error:", err);
            res.write(JSON.stringify({ error: err.message }) + '\n');
        } else {
            console.log("Pipeline Finished.");
            res.write(JSON.stringify({ done: true }) + '\n');
        }
        // 执行完毕，关闭 HTTP 连接
        res.end();
    });
});

server.listen(3000, () => {
    console.log('Astra Bridge Server is running on http://localhost:3000');
});
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
    const { nodes = [], edges = [] } = req.body || {};

    // 明确写入流式协议头，关闭缓存，并防止 Express 内部自动干扰
    res.writeHead(200, {
        'Content-Type': 'application/x-ndjson',
        'Transfer-Encoding': 'chunked',
        'Connection': 'keep-alive',
        'Cache-Control': 'no-cache'
    });


    console.log("Building pipeline for new request...");
    const pipeline = new astra.Pipeline();

    // 放入 try-catch 防止显存 OOM 导致服务器崩溃
    try {
        if (!Array.isArray(nodes) || nodes.length === 0) {
            throw new Error('Invalid graph: nodes is required.');
        }

        const nodeIds = new Set();
        const inferenceNodeIds = new Set();

        for (const node of nodes) {
            if (!node?.id || !node?.type) {
                throw new Error('Invalid node entry in graph payload.');
            }
            if (nodeIds.has(node.id)) {
                throw new Error(`Duplicate node id: ${node.id}`);
            }
            nodeIds.add(node.id);

            if (node.type === 'formatNode') {
                const prompt = node?.data?.formatStr || 'User: Hello\nAssistant:';
                pipeline.addFormatNode(node.id, String(prompt));
            } else if (node.type === 'inferenceNode') {
                pipeline.addInferenceNode(node.id);
                inferenceNodeIds.add(node.id);
            } else if (node.type === 'outputNode') {
                pipeline.addOutputNode(node.id);
            }
        }

        const incomingCount = new Map();

        for (const edge of edges) {
            if (!edge?.source || !edge?.target) continue;
            if (!nodeIds.has(edge.source) || !nodeIds.has(edge.target)) continue;
            pipeline.addEdge(edge.source, edge.target);
            incomingCount.set(edge.target, (incomingCount.get(edge.target) || 0) + 1);
        }

        for (const inferId of inferenceNodeIds) {
            if ((incomingCount.get(inferId) || 0) === 0) {
                throw new Error(`Inference node ${inferId} has no upstream input.`);
            }
        }
    } catch (e) {
        console.error("Pipeline initialization failed (Likely OOM):", e);
        res.write(JSON.stringify({ error: e.message }) + '\n');
        res.end();
        pipeline.dispose(); // 初始化失败也要释放
        return;
    }

    // 绑定 Token 流水线回调
    pipeline.onToken((nodeId, text) => {
        // 每生成一个 Token，就立刻转成 JSON 字符串并加一个换行符推给前端
        const chunk = JSON.stringify({ nodeId, text }) + '\n';
        res.write(chunk);
    });

    let isFinished = false;
    let isClientDisconnected = false;

    // NOTE:
    // req.close 在请求体读取完成后也可能触发，不能用它来判断“客户端中断”。
    // 改为监听 response 的 close，并结合 writableEnded 判断是否是异常断开。
    res.on('close', () => {
        const disconnectedEarly = !res.writableEnded;
        if (!isFinished && disconnectedEarly) {
            isClientDisconnected = true;
            console.log("Client aborted response stream, stopping pipeline...");
            pipeline.stop();
        }
    });

    // 开始执行
    pipeline.run((err, result) => {
        isFinished = true;
        if (isClientDisconnected) {
            console.log("Pipeline stopped because client disconnected.");
        } else if (err) {
            console.error("Pipeline Error:", err);
            res.write(JSON.stringify({ error: err.message }) + '\n');
        } else {
            console.log("Pipeline Finished.");
            res.write(JSON.stringify({ done: true }) + '\n');
        }

        // 客户端已断开时，避免继续写入已关闭的响应流
        if (!res.writableEnded) {
            // 执行完毕，关闭 HTTP 连接
            res.end();
        }

        // 推理结束，手动释放 C++ 内存，归还大模型 KV Context
        pipeline.dispose();
    });
});

server.listen(3000, () => {
    console.log('Astra Bridge Server is running on http://localhost:3000');
});

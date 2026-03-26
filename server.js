const express = require('express');
const cors = require('cors');
const path = require('path');
const http = require('http');
const fs = require('fs');
const fsp = fs.promises;
const crypto = require('crypto');
const multer = require('multer');
const WebSocket = require('ws');

// 引入 C++ 引擎
const astra = require('./build/Release/astrarp_node.node');

const app = express();

const configPath = path.resolve('./config.json');

const normalizeGraphConnectionConfig = (raw) => {
    const data = raw && typeof raw === 'object' ? raw : {};
    const configuredPath = typeof data.path === 'string' && data.path.trim() ? data.path : './graph_connections.json';
    return {
        enabled: data.enabled !== false,
        path: path.resolve(configuredPath),
        autoBuildBackend: data.auto_build_backend !== false,
        autoLoadFrontend: data.auto_load_frontend === true,
        allowFrontendSave: data.allow_frontend_save !== false
    };
};

const loadGlobalConfig = () => {
    try {
        const raw = fs.readFileSync(configPath, 'utf-8');
        return JSON.parse(raw);
    } catch (error) {
        console.warn('Failed to read config.json as JSON, fallback to defaults:', error?.message || error);
        return {};
    }
};

const globalJsonConfig = loadGlobalConfig();
const graphConnectionRaw = {
    ...(globalJsonConfig.graph_connection || {}),
    path: process.env.ASTRA_GRAPH_CONFIG_PATH || globalJsonConfig.graph_connection?.path
};
const graphConnectionConfig = normalizeGraphConnectionConfig(graphConnectionRaw);

const parseGraphPayload = (raw) => {
    if (!raw || typeof raw !== 'object') {
        throw new Error('Graph config payload must be an object.');
    }

    const nodes = Array.isArray(raw.nodes) ? raw.nodes : [];
    const edges = Array.isArray(raw.edges) ? raw.edges : [];

    return {
        nodes,
        edges
    };
};

const readGraphConfigFromFile = async () => {
    const raw = await fsp.readFile(graphConnectionConfig.path, 'utf-8');
    const parsed = JSON.parse(raw);
    return parseGraphPayload(parsed);
};

const writeGraphConfigToFile = async (graphPayload) => {
    const normalized = parseGraphPayload(graphPayload);
    await fsp.mkdir(path.dirname(graphConnectionConfig.path), { recursive: true });
    await fsp.writeFile(graphConnectionConfig.path, JSON.stringify(normalized, null, 2), 'utf-8');
};



const safelyInvokePipelineMethod = (pipeline, methodName, ...args) => {
    if (!pipeline || typeof pipeline[methodName] !== 'function') return false;
    pipeline[methodName](...args);
    return true;
};

const runtimeRoot = path.resolve('./runtime_loras');
const isPathWithin = (baseDir, targetPath) => {
    const rel = path.relative(baseDir, targetPath);
    return rel && !rel.startsWith('..') && !path.isAbsolute(rel);
};

const normalizeLoraPath = (rawPath) => {
    if (typeof rawPath !== 'string' || !rawPath.trim()) return undefined;
    const resolved = path.resolve(rawPath);
    if (!isPathWithin(runtimeRoot, resolved)) {
        throw new Error('LoRA path is not allowed.');
    }
    return resolved;
};

const sanitizeInferenceConfig = (raw) => {
    const data = raw && typeof raw === 'object' ? raw : {};
    return {
        addSpecial: typeof data.addSpecial === 'boolean' ? data.addSpecial : undefined,
        parseSpecial: typeof data.parseSpecial === 'boolean' ? data.parseSpecial : undefined,
        maxTokens: Number.isFinite(data.maxTokens) ? data.maxTokens : undefined,
        temperature: Number.isFinite(data.temperature) ? data.temperature : undefined,
        topK: Number.isFinite(data.topK) ? data.topK : undefined,
        topP: Number.isFinite(data.topP) ? data.topP : undefined,
        topPMinKeep: Number.isFinite(data.topPMinKeep) ? data.topPMinKeep : undefined,
        seed: Number.isFinite(data.seed) ? data.seed : undefined,
        grammar: typeof data.grammar === 'string' ? data.grammar : undefined,
        loraName: typeof data.loraName === 'string' ? data.loraName : undefined,
        loraPath: normalizeLoraPath(data.loraPath),
        loraScale: Number.isFinite(data.loraScale) ? data.loraScale : undefined
    };
};

app.use(cors());
app.use(express.json({ limit: '2mb' }));


const loraUpload = multer({
    storage: multer.diskStorage({
        destination: (req, _file, cb) => {
            try {
                const runId = `${Date.now()}_${crypto.randomBytes(4).toString('hex')}`;
                req.loraRunId = runId;
                const targetDir = path.join(runtimeRoot, runId);
                fs.mkdirSync(targetDir, { recursive: true });
                cb(null, targetDir);
            } catch (err) {
                cb(err);
            }
        },
        filename: (req, file, cb) => {
            const fileName = safeName(file.originalname || 'adapter.gguf');
            req.loraFileName = fileName;
            cb(null, fileName);
        }
    }),
    limits: { fileSize: 1024 * 1024 * 1024 * 2 },
    fileFilter: (_req, file, cb) => {
        const fileName = safeName(file.originalname || 'adapter.gguf');
        if (!fileName.toLowerCase().endsWith('.gguf')) {
            return cb(new Error('LoRA file must be .gguf'));
        }
        return cb(null, true);
    }
});

const safeName = (name) => String(name || '').replace(/[^a-zA-Z0-9._-]/g, '_');
const sendApiError = (res, code, stage, error, status = 400) => {
    const message = error instanceof Error ? error.message : String(error || 'Unknown error');
    return res.status(status).json({ error: message, code, stage });
};

app.post('/api/lora/upload', loraUpload.single('loraFile'), async (req, res) => {
    try {
        if (!req.file) {
            return sendApiError(res, 'LORA_FILE_MISSING', 'lora.upload', 'Missing loraFile', 400);
        }

        const fileName = safeName(req.file.originalname || 'adapter.gguf');
        if (!fileName.toLowerCase().endsWith('.gguf')) {
            return sendApiError(res, 'LORA_FILE_INVALID', 'lora.upload', 'LoRA file must be .gguf', 400);
        }

        const loraName = safeName(req.body?.loraName || fileName.replace(/\.gguf$/i, ''));
        const targetDir = req.file?.destination || path.join(runtimeRoot, String(req.loraRunId || ''));
        const targetPath = path.join(targetDir, fileName);

        return res.json({ loraName, loraPath: targetPath });
    } catch (error) {
        console.error('LoRA upload failed:', error);
        return sendApiError(res, 'LORA_UPLOAD_FAILED', 'lora.upload', 'LoRA upload failed', 500);
    }
});

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

    console.log("Initializing Astra Engine with:", configPath);
    astra.initSystem(configPath);
    console.log("Astra Engine initialized successfully.");
} catch (err) {
    console.error("Failed to init system:", err);
    process.exit(1);
}

app.get('/api/graph-config', async (_req, res) => {
    if (!graphConnectionConfig.enabled) {
        return sendApiError(res, 'GRAPH_CONFIG_DISABLED', 'graph.read', 'Graph connection config is disabled in global config.', 404);
    }

    try {
        const graph = await readGraphConfigFromFile();
        return res.json({
            config: {
                ...graphConnectionConfig,
                path: graphConnectionConfig.path
            },
            graph
        });
    } catch (error) {
        const code = error && error.code === 'ENOENT' ? 404 : 500;
        const message = code === 404 ? 'Graph config file not found.' : 'Failed to read graph config.';
        return res.status(code).json({
            error: message,
            code: code === 404 ? 'GRAPH_CONFIG_NOT_FOUND' : 'GRAPH_CONFIG_READ_FAILED',
            stage: 'graph.read',
            config: {
                ...graphConnectionConfig,
                path: graphConnectionConfig.path
            }
        });
    }
});

app.post('/api/graph-config', async (req, res) => {
    if (!graphConnectionConfig.enabled) {
        return sendApiError(res, 'GRAPH_CONFIG_DISABLED', 'graph.write', 'Graph connection config is disabled in global config.', 404);
    }
    if (!graphConnectionConfig.allowFrontendSave) {
        return sendApiError(res, 'GRAPH_CONFIG_SAVE_DISABLED', 'graph.write', 'Saving graph config from frontend is disabled.', 403);
    }

    try {
        await writeGraphConfigToFile(req.body || {});
        return res.json({ ok: true, path: graphConnectionConfig.path });
    } catch (error) {
        return sendApiError(res, 'GRAPH_CONFIG_SAVE_FAILED', 'graph.write', error instanceof Error ? error.message : 'Failed to save graph config.', 400);
    }
});

// 2. 提供流式推理 API
app.post('/api/run', (req, res) => {
    const hasGraphPayload = Array.isArray(req.body?.nodes);

    const runWithGraph = (graph) => {
        const { nodes = [], edges = [] } = graph || {};

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
                pipeline.addInferenceNode(node.id, sanitizeInferenceConfig(node?.data));
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
        res.write(JSON.stringify({ error: e.message, code: 'PIPELINE_INIT_FAILED', stage: 'pipeline.init' }) + '\n');
        res.end();
        safelyInvokePipelineMethod(pipeline, "dispose"); // 兼容无 dispose 版本绑定
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
            safelyInvokePipelineMethod(pipeline, "stop");
        }
    });

    // 开始执行
    pipeline.run((err, result) => {
        isFinished = true;
        if (isClientDisconnected) {
            console.log("Pipeline stopped because client disconnected.");
        } else if (err) {
            console.error("Pipeline Error:", err);
            res.write(JSON.stringify({ error: err.message, code: 'PIPELINE_RUN_FAILED', stage: 'pipeline.run' }) + '\n');
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
        safelyInvokePipelineMethod(pipeline, "dispose");
    });
    };

    if (!hasGraphPayload && graphConnectionConfig.enabled && graphConnectionConfig.autoBuildBackend) {
        readGraphConfigFromFile()
            .then((graph) => runWithGraph(graph))
            .catch((error) => {
                res.write(JSON.stringify({
                    error: `Failed to auto load graph config: ${error.message || error}`,
                    code: 'GRAPH_CONFIG_AUTOLOAD_FAILED',
                    stage: 'graph.read'
                }) + '\n');
                res.end();
            });
        return;
    }

    runWithGraph(req.body || {});
});

server.listen(3000, () => {
    console.log('Astra Bridge Server is running on http://localhost:3000');
});

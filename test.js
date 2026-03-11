// 假设编译后的模块在 ./build/Release/astra_rp.node
const astra = require('./build/Release/astrarp_node.node');

console.log("--- 模块导出的 API 列表 ---");
console.log(Object.keys(astra));
// 你应该能看到: ['initSystem', 'Pipeline']

/**
 * 详细解释：
 * 1. initSystem: 这是你在 binding.cpp 的 InitAll 中导出的全局静态函数。
 *    用于一次性初始化底层资源（如模型加载器、缓存池）。
 * 
 * 2. Pipeline: 这是一个类（由 Napi::ObjectWrap 包装）。
 *    每次 new astra.Pipeline() 都会在 C++ 中创建一个新的 Graph 和 EventBus 实例。
 */

async function runTest() {
    // 步骤 1: 初始化全局环境
    try {
        console.log("\n--- 初始化系统 ---");
        astra.initSystem("./config.json");
    } catch (err) {
        console.error("初始化失败:", err.message);
        return;
    }

    // 步骤 2: 实例化流水线
    const pipeline = new astra.Pipeline();

    // 步骤 3: 绑定事件监听 (核心交互)
    pipeline.onToken((nodeId, text) => {
        // 这里是你的 EventBus 实时推送的地方
        process.stdout.write(text);
    });

    // 步骤 4: 构建 DAG
    console.log("\n--- 构建流水线 ---");
    pipeline.addFormatNode("prompt_node", "你好，请自我介绍一下。");
    pipeline.addInferenceNode("infer_node");
    pipeline.addEdge("prompt_node", "infer_node");

    // 步骤 5: 异步运行
    console.log("\n--- 开始执行 ---");
    return new Promise((resolve, reject) => {
        pipeline.run((err, res) => {
            if (err) reject(err);
            else resolve(res);
        });
    });
}

runTest()
    .then(() => console.log("\n测试完成"))
    .catch(console.error);
const { execSync } = require("child_process");
const path = require("path");
const fs = require("fs");

function run(cmd) {
    console.log(`\n>> ${cmd}`);
    execSync(cmd, { stdio: "inherit" });
}

// 递归查找指定后缀的文件
function findFiles(dir, ext, fileList = []) {
    if (!fs.existsSync(dir)) return fileList;
    const files = fs.readdirSync(dir);
    files.forEach(file => {
        const filePath = path.join(dir, file);
        if (fs.statSync(filePath).isDirectory()) {
            findFiles(filePath, ext, fileList);
        } else if (file.toLowerCase().endsWith(ext)) {
            fileList.push(filePath);
        }
    });
    return fileList;
}

try {
    // const root = process.cwd();
    process.chdir("..");
    const parent = process.cwd();
    const coreDir = path.join(parent, "core");

    // 1. 编译 CMake (Core)
    process.chdir(coreDir);
    console.log("Building core with CMake...");
    run("cmake -B build");
    run("cmake --build build --config Release");

    // 2. 自动收集所有 .lib 文件，重写 binding.gyp
    process.chdir(parent);
    console.log("\nAuto-configuring binding.gyp...");
    const buildDir = path.join(coreDir, "build");

    // 寻找所有的 .lib 文件（静态库 或 DLL导入库）
    const libs = findFiles(buildDir, ".lib")
        // 排除掉测试相关的库，避免符号冲突
        .filter(lib => !lib.includes("benchmark") && !lib.includes("gtest"))
        // 将绝对路径转为相对于 binding.gyp 的路径，并将 Windows 的 \ 换成 /
        .map(lib => path.relative(parent, lib).replace(/\\/g, '/'));

    console.log("Found libraries for linking:\n  " + libs.join("\n  "));

    // 动态生成 binding.gyp 的内容
    const bindingGypConfig = {
        targets: [
            {
                target_name: "astrarp_node",
                sources: ["core/binding.cpp"], // 确保你的 binding.cpp 在这个路径
                include_dirs: [
                    "<!@(node -p \"require('node-addon-api').include\")",
                    "./core/include",
                    "./core/llama.cpp/ggml/include",
                    "./core/llama.cpp/include",
                    "./core/pjh_json/include"
                ],
                dependencies: ["<!(node -p \"require('node-addon-api').gyp\")"],
                libraries: libs.map(lib => `<(module_root_dir)/${lib}`), // 使用 module_root_dir 保证路径绝对正确
                cflags_cc: ["-std=c++20"],
                // 如果你的 llama.cpp 最终还是输出了 DLL，加上 "LLAMA_SHARED" 可以消除 __imp_ 报错
                defines: ["NAPI_CPP_EXCEPTIONS", "LLAMA_SHARED"],
                msvs_settings: {
                    VCCLCompilerTool: {
                        DebugInformationFormat: "0",
                        ExceptionHandling: 1,
                        AdditionalOptions: ["/utf-8"]
                    }
                }
            }
        ]
    };

    fs.writeFileSync(
        path.join(parent, "binding.gyp"),
        JSON.stringify(bindingGypConfig, null, 4)
    );
    console.log("binding.gyp has been successfully generated!");

    // 3. 构建 Node Addon
    console.log("\nBuilding node addon...");
    run("node-gyp clean");
    run("node-gyp configure");
    run("node-gyp build");

    // 4. 自动拷贝运行所需的 DLL (如果有的话)
    const targetDir = path.join(parent, "build", "Release");
    console.log(`\nSearching for runtime DLLs to copy to ${targetDir}...`);
    const dlls = findFiles(buildDir, ".dll");

    if (dlls.length > 0) {
        if (!fs.existsSync(targetDir)) {
            fs.mkdirSync(targetDir, { recursive: true });
        }
        dlls.forEach(dllPath => {
            const destPath = path.join(targetDir, path.basename(dllPath));
            fs.copyFileSync(dllPath, destPath);
            console.log(`Copied: ${path.basename(dllPath)}`);
        });
    } else {
        console.log("No DLLs found (Pure static build). This is great!");
    }

    console.log("\nAll Build finished successfully! 🎉");
} catch (err) {
    console.error("\nBuild failed ❌:");
    console.error(err.message);
    process.exit(1);
}
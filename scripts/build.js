const { execSync } = require("child_process");
const path = require("path");
const fs = require("fs");
const os = require("os");

function run(cmd, cwd) {
    console.log(`\n>> ${cmd}`);
    execSync(cmd, { stdio: "inherit", cwd: cwd });
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
    // ---------------------------------------------------------
    // 1. 环境与路径准备 (更安全的绝对路径定位)
    // ---------------------------------------------------------
    const parentDir = path.resolve(__dirname, "..");
    const coreDir = path.join(parentDir, "core");

    console.log(`Project root: ${parentDir}`);
    console.log(`Core dir: ${coreDir}`);

    // 判断操作系统
    const isWin = process.platform === "win32";
    const isMac = process.platform === "darwin";

    // 动态设定库后缀名
    const staticLibExt = isWin ? ".lib" : ".a";
    const sharedLibExt = isWin ? ".dll" : (isMac ? ".dylib" : ".so");

    const buildPath = path.join(coreDir, "build");

    if (fs.existsSync(buildPath)) {
        console.log("\n[!] Cleaning old build directory...");
        // 跨平台删除目录
        fs.rmSync(buildPath, { recursive: true, force: true });
    }

    // ---------------------------------------------------------
    // 2. 子模块自动检测与拉取
    // ---------------------------------------------------------
    const llamaCheckFile = path.join(coreDir, "llama.cpp", "CMakeLists.txt");
    const jsonCheckFile = path.join(coreDir, "pjh_json", "CMakeLists.txt");

    if (!fs.existsSync(llamaCheckFile) || !fs.existsSync(jsonCheckFile)) {
        console.log("\n[!] Submodules missing. Auto-initializing git submodules...");
        // 在项目根目录执行 git submodule 初始化
        run("git submodule update --init --recursive", parentDir);
    } else {
        console.log("\n[+] Submodules already initialized.");
    }

    // ---------------------------------------------------------
    // 3. 编译 CMake (Core)
    // ---------------------------------------------------------
    console.log("\n[+] Building core with CMake...");
    // 注意: Linux/macOS 的生成器默认是单配置，必须在这里加上 -DCMAKE_BUILD_TYPE=Release
    run("cmake -B build -DCMAKE_BUILD_TYPE=Release", coreDir);
    run("cmake --build build --config Release", coreDir);

    // ---------------------------------------------------------
    // 4. 自动收集构建出来的静态库，重写 binding.gyp
    // ---------------------------------------------------------
    console.log("\n[+] Auto-configuring binding.gyp...");
    const buildDir = path.join(coreDir, "build");

    // 寻找所有的静态库文件 (.lib 或 .a)
    const libs = findFiles(buildDir, staticLibExt)
        // 排除掉测试相关的库，避免符号冲突
        .filter(lib => !lib.includes("benchmark") && !lib.includes("gtest") && !lib.includes("mock"))
        // 将绝对路径转为相对于 binding.gyp 的路径，统一换成 POSIX 风格的正斜杠 (node-gyp 通用)
        .map(lib => path.relative(parentDir, lib).replace(/\\/g, '/'));

    if (libs.length === 0) {
        throw new Error(`No static libraries (${staticLibExt}) found in ${buildDir}! Compilation might have failed.`);
    }

    console.log("Found libraries for linking:\n  " + libs.join("\n  "));

    // 动态生成 binding.gyp 的内容
    const bindingGypConfig = {
        targets: [
            {
                target_name: "astrarp_node",
                sources: ["core/binding.cpp"],
                include_dirs: [
                    "<!@(node -p \"require('node-addon-api').include\")",
                    "./core/include",
                    "./core/llama.cpp/ggml/include",
                    "./core/llama.cpp/include",
                    "./core/pjh_json/include",
                    "./core/pjh_json/include/pjh_json"
                ],
                dependencies: ["<!(node -p \"require('node-addon-api').gyp\")"],
                // <(module_root_dir) 是 node-gyp 内置变量，代表 binding.gyp 所在的根目录
                libraries: libs.map(lib => `<(module_root_dir)/${lib}`),

                // 跨平台编译标志
                cflags_cc: ["-std=c++20", "-O3", "-march=native"],
                defines: ["NAPI_CPP_EXCEPTIONS", "LLAMA_SHARED"],

                // node-gyp 在 Linux/macOS 会自动忽略 msvs_settings，所以放这里很安全
                msvs_settings: {
                    VCCLCompilerTool: {
                        RuntimeLibrary: 0,
                        DebugInformationFormat: "0",
                        ExceptionHandling: 1,
                        AdditionalOptions: ["/utf-8"]
                    }
                }
            }
        ]
    };

    fs.writeFileSync(
        path.join(parentDir, "binding.gyp"),
        JSON.stringify(bindingGypConfig, null, 4)
    );
    console.log("[+] binding.gyp has been successfully generated!");

    // ---------------------------------------------------------
    // 5. 构建 Node Addon
    // ---------------------------------------------------------
    console.log("\n[+] Building node addon...");
    run("node-gyp clean", parentDir);
    run("node-gyp configure", parentDir);
    run("node-gyp build", parentDir);

    // ---------------------------------------------------------
    // 6. 自动拷贝运行所需的 动态库 (如果有)
    // ---------------------------------------------------------
    const targetDir = path.join(parentDir, "build", "Release");
    console.log(`\n[+] Searching for runtime shared libraries (${sharedLibExt}) to copy to ${targetDir}...`);

    const sharedLibs = findFiles(buildDir, sharedLibExt);

    if (sharedLibs.length > 0) {
        if (!fs.existsSync(targetDir)) {
            fs.mkdirSync(targetDir, { recursive: true });
        }
        sharedLibs.forEach(libPath => {
            const destPath = path.join(targetDir, path.basename(libPath));
            fs.copyFileSync(libPath, destPath);
            console.log(`  -> Copied: ${path.basename(libPath)}`);
        });
    } else {
        console.log(`  -> No shared libraries (${sharedLibExt}) found. (Pure static build, this is great!)`);
    }

    console.log("\n=============================================");
    console.log(" 🎉 All Build finished successfully! 🎉");
    console.log("=============================================\n");

} catch (err) {
    console.error("\n❌ Build failed:");
    console.error(err.message);
    process.exit(1);
}
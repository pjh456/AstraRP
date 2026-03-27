// build.js
const { execSync } = require("child_process");
const path = require("path");
const fs = require("fs");

function run(cmd, cwd, env = process.env) {
    console.log(`\n>> ${cmd}`);
    execSync(cmd, { stdio: "inherit", cwd: cwd, env: env });
}

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

    const currentDir = __dirname;
    const dirName = path.basename(currentDir);

    // 判断是否在 scripts 目录
    const rootDir =
        dirName === "scripts"
            ? path.resolve(currentDir, "..")
            : currentDir;

    console.log(`[+] Script directory: ${currentDir}`);
    console.log(`[+] Project root: ${rootDir}`);

    const coreDir = path.join(rootDir, "core");
    const buildPath = path.join(coreDir, "build");

    const staticLibExts = [".a", ".lib"];

    // 1. 强制清理旧编译产物
    if (fs.existsSync(buildPath)) {
        console.log("[!] Cleaning old build directory...");
        fs.rmSync(buildPath, { recursive: true, force: true });
    }

    const cmakeEnv = Object.assign({}, process.env);
    if (process.platform === "win32") {
        // 清除任何强行指定的全局生成器或编译器
        delete cmakeEnv.CMAKE_GENERATOR;
        delete cmakeEnv.CC;
        delete cmakeEnv.CXX;

        // 动态查找 PATH 键名 (无视大小写)
        const pathKey = Object.keys(cmakeEnv).find(k => k.toLowerCase() === "path");
        if (pathKey && cmakeEnv[pathKey]) {
            cmakeEnv[pathKey] = cmakeEnv[pathKey]
                .split(path.delimiter)
                .filter(p => {
                    // 将路径统一转为小写且斜杠统一，防止误伤含有这些字符的用户名
                    const normalizedPath = p.toLowerCase().replace(/\\/g, '/');
                    return !normalizedPath.includes('/mingw') && 
                           !normalizedPath.includes('/msys') && 
                           !normalizedPath.includes('/cygwin') && 
                           !normalizedPath.includes('/ninja') &&
                           !normalizedPath.includes('ccache');
                })
                .join(path.delimiter);
            console.log("[*] Cleaned PATH for CMake to force Visual Studio usage.");
        }
    }

    // 2. 编译 Core CMake
    console.log("[+] Building core with CMake...");
    try {
        run("cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CONFIGURATION_TYPES=Release", coreDir, cmakeEnv);
        run("cmake --build build --config Release --parallel", coreDir, cmakeEnv);
    } catch (e) {
        if (e.message && e.message.includes("is not recognized")) {
            console.error("\n[!] 错误: CMake 找不到！这说明你目前使用的 CMake 可能安装在 MinGW 里。");
            console.error("[!] 解决: 请去 CMake 官网下载并安装 Windows 官方版 CMake 并加入环境变量。");
            process.exit(1);
        }
        throw e;
    }

    // 3. 生成 binding.gyp
    console.log("[+] Auto-configuring binding.gyp...");
    const libs = findFiles(buildPath, "")
    .filter(file => staticLibExts.some(ext => file.endsWith(ext)))

    if (libs.length === 0) throw new Error("No static libraries found!");

    const bindingGypConfig = {
        targets: [{
            target_name: "astrarp_node",
            sources: ["core/binding.cpp"],
            include_dirs: [
                "<!@(node -p \"require('node-addon-api').include\")",
                "./core/include",
                "./core/llama.cpp/ggml/include",
                "./core/llama.cpp/include",
                "./core/pjh_json/include"
            ],
            dependencies: ["<!(node -p \"require('node-addon-api').gyp\")"],
            libraries: libs,
            cflags_cc: ["-std=c++20", "-O3", "-march=native", "-fexceptions"],
            defines: ["NAPI_CPP_EXCEPTIONS", "LLAMA_SHARED"],
            msvs_settings: {
                VCCLCompilerTool: {
                    RuntimeLibrary: 0, // 0 = /MT (静态运行时)，解决 LNK2038 报错
                    DebugInformationFormat: 0,
                    ExceptionHandling: 1,
                    AdditionalOptions: ["/utf-8"]
                }
            }
        }]
    };

    fs.writeFileSync(path.join(rootDir, "binding.gyp"), JSON.stringify(bindingGypConfig, null, 4));

    // 4. 构建 Node Addon
    console.log("[+] Building node addon...");
    run("node-gyp clean", rootDir);
    run("node-gyp configure", rootDir);
    run("node-gyp build", rootDir);

    // 5. 复制动态库
    const targetDir = path.join(rootDir, "build", "Release");
    // const sharedLibs = findFiles(buildPath, ".dll"); 
    // sharedLibs.forEach(libPath => {
    //     if (!fs.existsSync(targetDir)) fs.mkdirSync(targetDir, { recursive: true });
    //     fs.copyFileSync(libPath, path.join(targetDir, path.basename(libPath)));
    //     console.log(`  -> Copied: ${path.basename(libPath)}`);
    // });

    console.log("\n✅ All Build finished successfully!");

} catch (err) {
    console.error("\n❌ Build failed:", err.message);
    process.exit(1);
}

// build.js
const { execSync } = require("child_process");
const path = require("path");
const fs = require("fs");

function run(cmd, cwd) {
    console.log(`\n>> ${cmd}`);
    execSync(cmd, { stdio: "inherit", cwd: cwd });
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

    const isWin = process.platform === "win32";
    const staticLibExt = isWin ? ".lib" : ".a";
    const sharedLibExt = isWin ? ".dll" : (process.platform === "darwin" ? ".dylib" : ".so");

    // 1. 强制清理旧编译产物
    if (fs.existsSync(buildPath)) {
        console.log("[!] Cleaning old build directory...");
        fs.rmSync(buildPath, { recursive: true, force: true });
    }

    // 2. 编译 Core CMake
    console.log("[+] Building core with CMake...");
    run("cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CONFIGURATION_TYPES=Release", coreDir);
    run("cmake --build build --config Release", coreDir);

    // 3. 生成 binding.gyp
    console.log("[+] Auto-configuring binding.gyp...");
    const libs = findFiles(buildPath, staticLibExt)
        .filter(lib => !lib.includes("benchmark") && !lib.includes("gtest") && !lib.includes("mock"))
        .map(lib => path.relative(rootDir, lib).replace(/\\/g, '/'));

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
            libraries: libs.map(lib => `<(module_root_dir)/${lib}`),
            cflags_cc: ["-std=c++20", "-O3", "-march=native"],
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
    const sharedLibs = findFiles(buildPath, sharedLibExt);
    sharedLibs.forEach(libPath => {
        if (!fs.existsSync(targetDir)) fs.mkdirSync(targetDir, { recursive: true });
        fs.copyFileSync(libPath, path.join(targetDir, path.basename(libPath)));
        console.log(`  -> Copied: ${path.basename(libPath)}`);
    });

    console.log("\n✅ All Build finished successfully!");

} catch (err) {
    console.error("\n❌ Build failed:", err.message);
    process.exit(1);
}
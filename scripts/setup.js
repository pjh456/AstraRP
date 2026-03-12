// setup.js
const { execSync } = require("child_process");
const path = require("path");
const fs = require("fs");

function run(cmd, cwd) {
    console.log(`\n>> ${cmd}`);
    execSync(cmd, { stdio: "inherit", cwd: cwd });
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

    console.log("[+] Initializing AstraRP Development Environment...");

    // 0. 删除 binding.gyp（避免 npm install 触发 node-gyp）
    const gypFile = path.join(rootDir, "binding.gyp");
    if (fs.existsSync(gypFile)) {
        console.log("[+] Removing binding.gyp to avoid node-gyp rebuild...");
        fs.unlinkSync(gypFile);
    }

    // 1. 初始化 Git 子模块
    const gitModulesFile = path.join(rootDir, ".gitmodules");
    if (fs.existsSync(gitModulesFile)) {
        console.log("[+] Updating submodules...");
        run("git submodule update --init --recursive", rootDir);
    } else {
        console.warn("[!] No .gitmodules found, skipping submodule sync.");
    }

    // 2. 安装 Node.js 依赖
    console.log("[+] Installing npm dependencies...");
    run("npm install", rootDir);

    console.log("\n=============================================");
    console.log(" 🎉 Environment setup finished!");
    console.log("    Next step: run 'node build.js' to compile.");
    console.log("=============================================\n");

} catch (err) {
    console.error("\n❌ Setup failed:");
    console.error(err.message);
    process.exit(1);
}
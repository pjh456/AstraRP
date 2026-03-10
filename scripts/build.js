const { execSync } = require("child_process");
const path = require("path");

function run(cmd) {
    console.log(`\n>> ${cmd}`);
    execSync(cmd, { stdio: "inherit" });
}

try {
    // 当前目录
    const root = process.cwd();

    // 回到上一级
    process.chdir("..");
    const parent = process.cwd();

    // 进入 core 文件夹
    const coreDir = path.join(parent, "core");
    process.chdir(coreDir);

    console.log("Building core...");

    run("cmake -B build");
    run("cmake --build build --config Release");

    // 回到上一级目录
    process.chdir(parent);

    console.log("Building node addon...");

    run("node-gyp clean");
    run("node-gyp configure");
    run("node-gyp build");

    console.log("\nBuild finished.");
} catch (err) {
    console.error("\nBuild failed:");
    console.error(err.message);
    process.exit(1);
}
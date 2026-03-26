// test.js
const { execSync } = require("child_process");
const path = require("path");
const fs = require("fs");

function run(cmd, cwd, env = process.env) {
    console.log(`\n>> ${cmd}`);
    execSync(cmd, { stdio: "inherit", cwd: cwd, env: env });
}

try {
    const currentDir = __dirname;
    const dirName = path.basename(currentDir);

    // 判断是否在 scripts 目录
    const rootDir =
        dirName === "scripts"
            ? path.resolve(currentDir, "..")
            : currentDir;

    const coreDir = path.join(rootDir, "core");
    const buildDir = path.join(coreDir, "build");

    console.log(`[+] Project root: ${rootDir}`);
    console.log(`[+] Core dir: ${coreDir}`);
    console.log(`[+] Build dir: ${buildDir}`);

    // 强制清理旧编译产物
    if (fs.existsSync(buildDir)) {
        console.log("[!] Cleaning old test build directory...");
        fs.rmSync(buildDir, { recursive: true, force: true });
    }

    const cmakeEnv = Object.assign({}, process.env);
    if (process.platform === "win32") {
        delete cmakeEnv.CMAKE_GENERATOR;
        delete cmakeEnv.CC;
        delete cmakeEnv.CXX;

        const pathKey = Object.keys(cmakeEnv).find((k) => k.toLowerCase() === "path");
        if (pathKey && cmakeEnv[pathKey]) {
            cmakeEnv[pathKey] = cmakeEnv[pathKey]
                .split(path.delimiter)
                .filter((p) => {
                    const normalizedPath = p.toLowerCase().replace(/\\/g, "/");
                    return (
                        !normalizedPath.includes("/mingw") &&
                        !normalizedPath.includes("/msys") &&
                        !normalizedPath.includes("/cygwin") &&
                        !normalizedPath.includes("/ninja") &&
                        !normalizedPath.includes("ccache")
                    );
                })
                .join(path.delimiter);
            console.log("[*] Cleaned PATH for CMake to force Visual Studio usage.");
        }
    }

    run(
        `cmake -B "${buildDir}" -DASTRARP_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CONFIGURATION_TYPES=Release`,
        coreDir,
        cmakeEnv
    );
    run(`cmake --build "${buildDir}" --config Release`, coreDir, cmakeEnv);
    run(`ctest --test-dir "${buildDir}" -C Release --output-on-failure`, coreDir, cmakeEnv);

    console.log("\n✅ Tests finished successfully!");
} catch (err) {
    console.error("\n❌ Tests failed:", err.message);
    process.exit(1);
}

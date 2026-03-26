// test.js
const { execSync } = require("child_process");
const path = require("path");

function run(cmd, cwd, env = process.env) {
    console.log(`\n>> ${cmd}`);
    execSync(cmd, { stdio: "inherit", cwd: cwd, env: env });
}

try {
    const currentDir = __dirname;
    const rootDir = path.resolve(currentDir, "..");
    const coreDir = path.join(rootDir, "core");
    const buildDir = path.join(coreDir, "build");

    console.log(`[+] Project root: ${rootDir}`);
    console.log(`[+] Core dir: ${coreDir}`);

    const cmakeEnv = Object.assign({}, process.env);
    if (process.platform === "win32") {
        delete cmakeEnv.CMAKE_GENERATOR;
        delete cmakeEnv.CC;
        delete cmakeEnv.CXX;
    }

    run(
        "cmake -B build -DASTRARP_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CONFIGURATION_TYPES=Release",
        coreDir,
        cmakeEnv
    );
    run("cmake --build build --config Release", coreDir, cmakeEnv);

    run(`ctest --test-dir "${buildDir}" -C Release --output-on-failure`, coreDir, cmakeEnv);

    console.log("\n✅ Tests finished successfully!");
} catch (err) {
    console.error("\n❌ Tests failed:", err.message);
    process.exit(1);
}

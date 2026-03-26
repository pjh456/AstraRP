// build_core.js
const { execSync } = require("child_process");
const path = require("path");
const fs = require("fs");

function run(cmd, cwd, env = process.env) {
    console.log(`\n>> ${cmd}`);
    execSync(cmd, { stdio: "inherit", cwd: cwd, env: env });
}

function parseArgs(argv) {
    const options = {};
    for (let i = 0; i < argv.length; i += 1) {
        const arg = argv[i];
        if (!arg.startsWith("--")) continue;
        const eq = arg.indexOf("=");
        if (eq !== -1) {
            options[arg.slice(2, eq)] = arg.slice(eq + 1);
            continue;
        }
        const key = arg.slice(2);
        const next = argv[i + 1];
        if (!next || next.startsWith("--")) {
            options[key] = true;
        } else {
            options[key] = next;
            i += 1;
        }
    }
    return options;
}

function parseBool(value) {
    if (typeof value === "boolean") return value;
    if (typeof value !== "string") return false;
    return ["1", "true", "yes", "on"].includes(value.toLowerCase());
}

try {
    const args = parseArgs(process.argv.slice(2));

    const currentDir = __dirname;
    const rootDir = path.resolve(currentDir, "..");
    const coreDir = path.join(rootDir, "core");
    const buildDir = path.resolve(args["build-dir"] || path.join(coreDir, "build"));

    const buildType = args["build-type"] || "Release";
    const config = args.config || buildType;
    const cmakeArgsExtra = args["cmake-args"] || "";
    const buildArgsExtra = args["build-args"] || "";
    const clean = parseBool(args.clean);

    console.log(`[+] Project root: ${rootDir}`);
    console.log(`[+] Core dir: ${coreDir}`);
    console.log(`[+] Build dir: ${buildDir}`);

    if (clean && fs.existsSync(buildDir)) {
        console.log("[!] Cleaning old build directory...");
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

    const configureArgs = [
        `-B "${buildDir}"`,
        `-DCMAKE_BUILD_TYPE=${buildType}`,
        `-DCMAKE_CONFIGURATION_TYPES=${config}`,
    ];
    if (cmakeArgsExtra) configureArgs.push(cmakeArgsExtra);

    run(`cmake ${configureArgs.join(" ")}`, coreDir, cmakeEnv);

    let buildCmd = `cmake --build "${buildDir}" --config ${config}`;
    if (buildArgsExtra) {
        buildCmd += ` -- ${buildArgsExtra}`;
    }
    run(buildCmd, coreDir, cmakeEnv);

    console.log("\n✅ Core build finished successfully!");
} catch (err) {
    console.error("\n❌ Core build failed:", err.message);
    process.exit(1);
}

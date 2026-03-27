// build_addon.js
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
    files.forEach((file) => {
        const filePath = path.join(dir, file);
        if (fs.statSync(filePath).isDirectory()) {
            findFiles(filePath, ext, fileList);
        } else if (file.toLowerCase().endsWith(ext)) {
            fileList.push(filePath);
        }
    });
    return fileList;
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
    const coreBuildDir = path.resolve(args["core-build-dir"] || path.join(coreDir, "build"));
    const gypClean = parseBool(args["gyp-clean"]);
    const configureArgs = args["configure-args"] || "";
    const buildArgs = args["build-args"] || "";

    console.log(`[+] Project root: ${rootDir}`);
    console.log(`[+] Core build dir: ${coreBuildDir}`);

    const staticLibExts = [".a", ".lib"];
    const libs = findFiles(coreBuildDir, "")
        .filter((file) => staticLibExts.some((ext) => file.endsWith(ext)));

    if (libs.length === 0) {
        throw new Error("No static libraries found. Build core first.");
    }

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
                    RuntimeLibrary: 0,
                    DebugInformationFormat: 0,
                    ExceptionHandling: 1,
                    AdditionalOptions: ["/utf-8"]
                }
            }
        }]
    };

    fs.writeFileSync(path.join(rootDir, "binding.gyp"), JSON.stringify(bindingGypConfig, null, 4));

    console.log("[+] Building node addon...");
    if (gypClean) {
        run("node-gyp clean", rootDir);
    }

    const configureCmd = configureArgs ? `node-gyp configure ${configureArgs}` : "node-gyp configure";
    const buildCmd = buildArgs ? `node-gyp build ${buildArgs}` : "node-gyp build";
    run(configureCmd, rootDir);
    run(buildCmd, rootDir);

    console.log("\n✅ Node addon build finished successfully!");
} catch (err) {
    console.error("\n❌ Node addon build failed:", err.message);
    process.exit(1);
}

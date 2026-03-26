// test_only.js
const { execSync } = require("child_process");
const path = require("path");

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

try {
    const args = parseArgs(process.argv.slice(2));

    const currentDir = __dirname;
    const rootDir = path.resolve(currentDir, "..");
    const coreDir = path.join(rootDir, "core");
    const buildDir = path.resolve(args["build-dir"] || path.join(coreDir, "build"));
    const config = args.config || "Release";
    const ctestArgs = args["ctest-args"] || "";

    console.log(`[+] Project root: ${rootDir}`);
    console.log(`[+] Build dir: ${buildDir}`);

    let ctestCmd = `ctest --test-dir "${buildDir}" -C ${config} --output-on-failure`;
    if (ctestArgs) {
        ctestCmd += ` ${ctestArgs}`;
    }

    run(ctestCmd, coreDir, process.env);
    console.log("\n✅ Tests finished successfully!");
} catch (err) {
    console.error("\n❌ Tests failed:", err.message);
    process.exit(1);
}

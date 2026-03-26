// test_rebuild.js
const { execSync } = require("child_process");
const path = require("path");

function run(cmd, cwd) {
    console.log(`\n>> ${cmd}`);
    execSync(cmd, { stdio: "inherit", cwd: cwd });
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

    const clean = args.clean ? "--clean" : "";
    const buildDirArg = args["build-dir"] ? `--build-dir "${args["build-dir"]}"` : "";
    const buildTypeArg = args["build-type"] ? `--build-type ${args["build-type"]}` : "";
    const configArg = args.config ? `--config ${args.config}` : "";
    const cmakeArgs = args["cmake-args"] || "";
    const ctestArgs = args["ctest-args"] || "";
    const buildArgs = args["build-args"] || "";

    let buildCoreCmd = `node scripts/build_core.js ${clean} ${buildDirArg} ${buildTypeArg} ${configArg}`;
    const cmakeArgsWithTests = cmakeArgs
        ? `${cmakeArgs} -DASTRARP_BUILD_TESTS=ON`
        : "-DASTRARP_BUILD_TESTS=ON";
    buildCoreCmd += ` --cmake-args "${cmakeArgsWithTests}"`;
    if (buildArgs) {
        buildCoreCmd += ` --build-args "${buildArgs}"`;
    }

    let testOnlyCmd = `node scripts/test_only.js ${buildDirArg} ${configArg}`;
    if (ctestArgs) {
        testOnlyCmd += ` --ctest-args "${ctestArgs}"`;
    }

    run(buildCoreCmd, rootDir);
    run(testOnlyCmd, rootDir);
} catch (err) {
    console.error("\n❌ Test rebuild failed:", err.message);
    process.exit(1);
}

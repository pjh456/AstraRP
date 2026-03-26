// test.js (legacy entry, keep for compatibility)
const { execSync } = require("child_process");
const path = require("path");

function run(cmd, cwd) {
    console.log(`\n>> ${cmd}`);
    execSync(cmd, { stdio: "inherit", cwd: cwd });
}

try {
    const rootDir = path.resolve(__dirname, "..");
    run("node scripts/test_rebuild.js --clean", rootDir);
} catch (err) {
    console.error("\n❌ Tests failed:", err.message);
    process.exit(1);
}

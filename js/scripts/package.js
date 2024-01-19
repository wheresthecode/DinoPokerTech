const fs = require("fs-extra");
const path = require("path");
const util = require("util");
const exec = util.promisify(require("child_process").exec);

async function build(outputDir) {
  try {
    if (!outputDir) {
      throw new Error("Output directory is required");
    }

    // Compile TypeScript files
    cmd = "tsc --outdir " + outputDir;
    console.log("Executing " + cmd);
    const { stdout, stderr } = await exec(cmd);
    console.log(stdout);
    if (stderr) {
      throw new Error(`Error compiling TypeScript: ${stderr}`);
    }

    // Copy .js and .wasm files
    var files = fs.readdirSync("./src").filter((fn) => fn.endsWith(".js") || fn.endsWith(".wasm"));
    console.log("Copying non-typescript files from src directory");
    files.forEach((element) => {
      console.log("./src/" + element + " -> " + outputDir + "/" + element);
      fs.copy("./src/" + element, outputDir + "/" + element);
    });

    // Copy and rename package.dist.json to package.json in output directory
    console.log("Generating package.json from package.dist.json");
    await fs.copy("package.dist.json", path.join(outputDir, "package.json"));

    console.log("Build and copy completed successfully");
  } catch (error) {
    console.error(`Build failed: ${error.message}`);
    process.exit(1);
  }
}

const outputDir = process.argv[2];
build(outputDir);

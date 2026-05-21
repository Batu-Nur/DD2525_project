const factory = require("./wasm.js");
const express = require("express");
const session = require('express-session')
const pug = require("pug");

const app = express();
const port = 3000;

app.use(session({ secret: "secret-sessions-key", resave: true, saveUninitialized: true }))
app.use(express.urlencoded({ extended: true }));
app.use(express.json());
app.use(express.static("public"));

// instance of the wasm module
let wasmInstance = null;

// functions from the wasm module
let get_nonce = null;
let get_script = null;

async function startApp() {
  try {
    wasmInstance = await factory();
    console.log("[+] WASM module loaded.");

    // defining wasm functions
    get_nonce = wasmInstance.cwrap("get_nonce", "number", []);
    get_script = wasmInstance.cwrap("get_script", "number", ["number", "number", "number"]);

    if (!wasmInstance) {
      throw new Error("[!] WASM module not initialized.");
    }
  } catch (error) {
    console.log(error);
    process.exit(1);
  }

  // start the express server after the wasm module has been initialized
  app.listen(port, () => {
    console.log(`[+] SSTI sample app running on localhost:${port}`);
  });
}

app.get("/", (req, res) => {
  let statusPtr = null;
  let noncePtr = null;

  try {
    if (!wasmInstance) {
      throw new Error("WASM instance not initialized when handling request.");
    }

    let { username } = req.query;
    if (!username) {
      username = "user"
    }

    statusPtr = wasmInstance._malloc(4);
    noncePtr = wasmInstance._malloc(8 * (16 + 1)); // 16 bytes for nonce + null terminator

    // getting the script from the wasm module
    let scriptPtr = get_script(wasmInstance.stringToNewUTF8(username), wasmInstance._malloc(4), noncePtr);
    let script = wasmInstance.UTF8ToString(scriptPtr);

    // sending the nonce to the client
    res.setHeader("Content-Security-Policy", `script-src 'nonce-${wasmInstance.UTF8ToString(noncePtr)}'`);

    const fn = pug.compile(`
doctype html
html
  head ${script}
  body
    h1 Welcome!
`);
    return res.send(fn({ username: username }))
  } catch (error) {
    console.error("[!] Error while handling request:", error);
    return res.status(500).send({
      ok: false,
      message: "Internal server error.",
    });
  } finally {
    if (statusPtr) wasmInstance._free(statusPtr);
    if (noncePtr) wasmInstance._free(noncePtr);
  }
});

// start the application
startApp();

// stop the application gracefully on SIGINT
process.on("SIGINT", () => {
  process.exit(0);
})

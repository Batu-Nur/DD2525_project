const factory = require('./wasm.js');
const express = require('express');
const fs = require('fs');
const path = require('path');
const bodyParser = require('body-parser')

const app = express();
const port = 3000;

app.use(bodyParser.json({ limit: '700mb' }))

// instance of the wasm module
let wasmInstance = null;
// pointer to the handler of the sqlite database
let dbPtr = null;

// functions from the wasm module
var get_user_secret = null;
var sqlite3_errmsg = null;
var sqlite3_exec = null;
var is_token_valid = null;

async function initializeDb() {
  if(!dbPtr) {
    console.error("[!] The dbPtr is not initialized");
    process.exit(1)
  }
   
  // randomly generated token for the admin user
  let adminToken = [...Array(10)].map(() => Math.random().toString(36)[2]).join('');
  let query = `CREATE TABLE IF NOT EXISTS users (
                 id INTEGER PRIMARY KEY,
                 token CHAR(10) NOT NULL UNIQUE,
                 secret VARCHAR(50) NOT NULL
               );
               INSERT INTO users (id, token, secret) VALUES 
               (1, '${adminToken}', 'supersecretinformation');
               INSERT INTO users (id, token, secret) VALUES 
               (2, '0123456789', 'notsosecretinformation');`;

  let queryPtr = wasmInstance.stringToNewUTF8(query);
  let errMsgRaw = wasmInstance.stringToNewUTF8("");

  let rc = sqlite3_exec(dbPtr, queryPtr, 0, 0, errMsgRaw);
  let errMsg = wasmInstance.getValue(errMsgRaw, "i32");

  if (rc !== 0) {
    console.error("[!] Error executing query to initialize DB:", wasmInstance.UTF8ToString(errMsg));
    process.exit(1);
  }

  console.log("[+] Database initialized successfully.");
  console.log("[+] Admin token:", adminToken);

  wasmInstance._free(queryPtr);
  wasmInstance._free(errMsg);
}

async function startApp() {
  let dbPathPtr = 0;
  let tmpDbPtr = 0;

  try {
    wasmInstance = await factory();
    console.log('[+] WASM module loaded.');

    get_user_secret = wasmInstance.cwrap(
      "get_user_secret", 
      'number',
      ['number', 'number', 'number'],
    );
    sqlite3_errmsg = wasmInstance.cwrap(
      "sqlite3_errmsg",
      "number",
      ['number'],
    );
    sqlite3_exec = wasmInstance.cwrap(
      "sqlite3_exec",
      "number",
      ["number", "number", "number", "number", "number"]
    );
    is_token_valid = wasmInstance.cwrap(
      "is_token_valid",
      "number",
      ["number", "number"]
    );

    
    // opening the sqlite db
    tmpDbPtr = wasmInstance._malloc(4);
    let rc = wasmInstance._open_database(":memory:", tmpDbPtr);

    // database failed to open
    if(!rc) {
      process.exit(1);
    }

    // getting the pointer to the database
    dbPtr = wasmInstance.getValue(tmpDbPtr, "i32");

    // initializing the database
    rc = initializeDb();
    if(!rc) {
      process.exit(1);
    }

    // checking that the logs directory exists and mount it to the wasm instance
    fs.mkdirSync('/tmp/logs', { recursive: true });
    wasmInstance.FS.mkdir("/tmp/logs");
    wasmInstance.FS.mount(wasmInstance.NODEFS, { root: '/tmp/logs' }, '/tmp/logs');
  } catch (error) {
    console.log(error);
    process.exit(1);
  } finally {
    // free all temporary pointers
    wasmInstance._free(dbPathPtr);
    wasmInstance._free(tmpDbPtr);
  }

  // start the express server after the wasm module has been initialized
  app.listen(port, () => {
    console.log(`[+] SQL-Injection sample app running on localhost:${port}`);
  });
}


app.post('/', (req, res) => {
  if (!wasmInstance) {
    console.error("[!] WASM instance not initialized when handling request.");
    return res.status(500).send({ ok: false, error: "Server not fully initialized." });
  }

  let token = req.body.token;

  if(token === undefined || token === null) {
    return res.status(400).send({ ok: false, error: "token is required" });
  }

  let token_ptr;
  let user_secret_ptr = 0;
  // allocate the space for the pointer to the status code
  // 1: user found
  // 0: user not found
  // -1: error
  let rc = wasmInstance._malloc(4);

  try {
    token_ptr = wasmInstance.stringToNewUTF8(token);

    if(dbPtr === null) {
      console.error("[!] Database pointer is null");
      process.exit(1);
    }

    user_secret = get_user_secret(dbPtr, token_ptr, rc);
    let status = wasmInstance.getValue(rc, "i32");

    switch(status) {
      case -1:
        console.error("[!] Error while checking user:", wasmInstance.UTF8ToString(sqlite3_errmsg(dbPtr)));
        break;
      case 0:
        res.status(403).send({"ok": false, "message": "Invalid token"});
        break;
      case 1:
        res.status(200).send({"ok": true, "secret": "no secret 4 u!"});
        // res.status(200).send({"ok": true, "secret": wasmInstance.UTF8ToString(user_secret)});
    }
  } catch (error) {
    console.error("[!] Error interacting with WASM during request:", error);
    res.status(500).send({ ok: false, error: "Error processing request."});
  } finally {
    // make sure to free the memory
    if (token_ptr) wasmInstance._free(token_ptr);
    if (rc) wasmInstance._free(rc);
    if (user_secret_ptr !== 0) wasmInstance._free(user_secret);
  }
});



app.get("/get-logs", (req, res) => {
  if (!wasmInstance) {
    console.error("[!] WASM instance not initialized when handling request.");
    return res.status(500).send({ ok: false, error: "Server not fully initialized." });
  }

  let token = req.query.token;

  if(!token) {
    return res.status(400).send({ ok: false, error: "token is required" });
  }

  let token_ptr = wasmInstance.stringToNewUTF8(token);

  if(is_token_valid(dbPtr, token_ptr) === -1) {
    console.error("[!] Error while checking token validity:", wasmInstance.UTF8ToString(sqlite3_errmsg(dbPtr)));
    wasmInstance._free(token_ptr);
    return res.status(500).send({"ok": false, "message": "Internal server error"});
  } else if(is_token_valid === 0) {
    wasmInstance._free(token_ptr);
    return res.status(403).send({"ok": false, "message": "Invalid token"});
  }

  // send the logs file in path "/tmp/logs/{token}.txt"
  res.sendFile(`/tmp/logs/users.log`, (err) => {
    if (err) {
      console.error("[!] Error sending logs file:", err);
      return res.status(500).send({ ok: false, error: "Error sending logs file." });
    }
  });
})

// start the application
startApp();


// logic to make sure that the sqlite db is properly closed
process.on('SIGINT', () => {
  console.log("[+] Shutting down application...");
  if (wasmInstance) {
    wasmInstance._close_db(dbPtr);
    wasmInstance._free(dbPtr);
    console.log('[+] WASM database closed.');
  }
  process.exit(0);
});

process.on('SIGTERM', () => {
  console.log("[+] Shutting down application...");
  if (wasmInstance) {
    wasmInstance._close_db(dbPtr);
    wasmInstance._free(dbPtr);
    console.log('[+] WASM database closed.');
  }
  process.exit(0);
});

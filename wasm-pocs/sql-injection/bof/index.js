const factory = require('./wasm.js');
const express = require('express');
const fs = require('fs');
const path = require('path');

const app = express();
const port = 3000;

app.use(express.urlencoded({ extended: true }));
app.use(express.json());

// instance of the wasm module
let wasmInstance = null;
// pointer to the handler of the sqlite database
let dbPtr = null;
// functions from the wasm module
var get_user_secret = null;

async function startApp() {
  let dbPathPtr = 0;
  let tmpDbPtr = 0;

  try {
    wasmInstance = await factory();
    console.log('[+] WASM module loaded.');

    get_user_secret = wasmInstance.cwrap(
      "get_user_secret", 
      'number',
      ['number', 'number', 'number', 'number'],
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
    rc = wasmInstance._initialize_db(dbPtr);
    if(!rc) {
      process.exit(1);
    }
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

  let username = req.body.username;
  let password = req.body.password;

  if(username === undefined || password === undefined) {
    return res.status(400).send({ ok: false, error: "username and password are required" });
  }

  let username_ptr = 0;
  let password_ptr = 0;
  let user_secret_ptr = 0;
  // allocate the space for the pointer to the status code
  // 1: user found
  // 0: user not found
  // -1: error
  let rc = wasmInstance._malloc(4);

  try {
    username_ptr = wasmInstance.stringToNewUTF8(username);
    password_ptr = wasmInstance.stringToNewUTF8(password);

    if(dbPtr === null) {
      console.error("[!] Database pointer is null");
      proccess.exit(1);
    }

    user_secret = get_user_secret(dbPtr, username_ptr, password_ptr, rc);
    let status = wasmInstance.getValue(rc, "i32");

    switch(status) {
      case -1:
        process.exit(1);
        break;
      case 0:
        res.status(403).send({"ok": false, "message": "Invalid username or password"});
        break;
      case 1:
        res.status(200).send({"ok": true, "secret": wasmInstance.UTF8ToString(user_secret)});
    }
  } catch (error) {
    console.error("[!] Error interacting with WASM during request:", error);
    res.status(500).send({ ok: false, error: "Error processing request."});
  } finally {
    // make sure to free the memory
    if (username_ptr) wasmInstance._free(username_ptr);
    if (password_ptr) wasmInstance._free(password_ptr);
    if (rc) wasmInstance._free(rc);
    if (user_secret_ptr !== 0) wasmInstance._free(user_secret);
  }
});

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

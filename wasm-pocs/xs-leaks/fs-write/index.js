const factory = require("./wasm.js");
const express = require("express");
const session = require('express-session')
const fs = require("fs");
const path = require("path");

const app = express();
const port = 3000;

app.use(session({ secret: "secret-sessions-key", resave: true, saveUninitialized: true }))
app.use(express.urlencoded({ extended: true }));
app.use(express.json());

// instance of the wasm module
let wasmInstance = null;

// functions from the wasm module
let add_user = null;
let get_secret = null;
let add_secret = null;
let edit_secret = null;
let debug = null;

async function startApp() {
  try {
    wasmInstance = await factory();
    console.log("[+] WASM module loaded.");

    // defining wasm functions
    add_user = wasmInstance.cwrap(
      "add_user",
      "void",
      ["int", "int"],
    );
    get_secret = wasmInstance.cwrap(
      "get_secret",
      "void",
      ["int", "int", "int"],
    );
    add_secret = wasmInstance.cwrap(
      "add_secret",
      "int",
      ["int", "int", "int"],
    );
    edit_secret = wasmInstance.cwrap(
      "edit_secret",
      "void",
      ["int", "int", "int", "int"],
    );
    debug = wasmInstance.cwrap(
      "debug",
      "void",
      []
    );

    if (!wasmInstance) {
      throw new Error("[!] WASM module not initialized.");
    }
  } catch (error) {
    console.log(error);
    process.exit(1);
  }

  // start the express server after the wasm module has been initialized
  app.listen(port, () => {
    console.log(`[+] XS-Leak sample app running on localhost:${port}`);
  });
}

// TODO: change this to a wrapper over the other endpoints
app.post("/register", (req, res) => {
  let idPtr = null;
  let secretPtr = null;

  try {
    if (!wasmInstance) {
      throw new Error("WASM instance not initialized when handling request.");
    }

    idPtr = wasmInstance._malloc(4);
    secretPtr = wasmInstance.stringToNewUTF8("");

    // registering the user with an empty secret
    add_user(secretPtr, idPtr)

    // store the user index in the session
    user_id = wasmInstance.getValue(idPtr, "i32");
    req.session.user_id = user_id;

    return res.status(200).send({
      ok: true,
      message: `User registered successfully with id ${user_id}.`,
    });
  } catch (error) {
    console.error("[!] Error while handling request:", error);
    return res.status(500).send({
      ok: false,
      message: "Internal server error.",
    });
  } finally {
    // free the allocated memory for the id pointer
    if (idPtr) wasmInstance._free(idPtr);
    if (secretPtr) wasmInstance._free(secretPtr);
  }
});

app.post("/add", (req, res) => {
  let secretPtr = null;

  try {
    if (req.session.user_id === undefined) {
      return res.status(403).send({
        ok: false,
        error: "You must register first.",
      });
    }

    if (!wasmInstance) {
      console.error("[!] WASM instance not initialized when handling request.");
      return res.status(500).send({
        ok: false,
        error: "Server not fully initialized.",
      });
    }

    let { secret, secretOffset } = req.body;
    if (!secret) {
      return res.status(400).send({
        ok: false,
        error: "Missing secret parameter.",
      });
    }

    secretPtr = wasmInstance.stringToNewUTF8(secret);

    if (add_secret(req.session.user_id, secretOffset == undefined ? -1 : secretOffset, secretPtr)) {
      return res.status(200).send({
        ok: true,
        message: "Secret added successfully.",
      });
    } else {
      throw new Error("Error while adding the secret.");
    }
  } catch (error) {
    console.error("[!] Error while handling request:", error);
    return res.status(500).send({
      ok: false,
      message: "Internal server error.",
    });
  } finally {
    if (secretPtr) wasmInstance._free(secretPtr);
  }
});

app.get("/search", (req, res) => {
  if (req.session.user_id === undefined) {
    return res.status(403).send({
      ok: false,
      error: "You must register first.",
    });
  }

  // pointer to store the status of the wasm function
  let statusPtr = null;
  let queryPtr = null;
  let secretPtr = null;

  try {
    if (!wasmInstance) {
      console.error("[!] WASM instance not initialized when handling request.");
      return res.status(500).send({
        ok: false,
        error: "Server not fully initialized.",
      });
    }

    // Extracting the query from the request body
    let { query } = req.query;
    if (!query) {
      return res.status(400).send({
        ok: false,
        error: "Missing 'query' parameter.",
      });
    }
    if (/[^a-zA-Z0-9\s]/.test(query)) {
      return res.status(400).send({
        ok: false,
        error: "Invalid characters in query.",
      });
    }

    // Calling the WASM function to retrieve a secret
    // the pointer is used to keep track of the status
    statusPtr = wasmInstance._malloc(4);
    queryPtr = wasmInstance.stringToNewUTF8(query);
    secretPtr = wasmInstance.stringToNewUTF8("");

    get_secret(req.session.user_id, queryPtr, statusPtr, secretPtr);
    let status = wasmInstance.getValue(statusPtr, "i32");

    switch (status) {
      // an error occurred
      case -2:
        console.log(wasmInstance.getValue(statusPtr, "i32"));
        throw new Error("Error while executing the WASM function.");
      // The forbidden words regex had a match
      case -1:
        return res.status(403).send({
          ok: false,
          message: "Forbidden input detected!",
        });
      // No secret found, either because it is private
      // or because the search regex had no match
      case 0:
        // no 404 to avoid XS-Leaks status-based
        return res.status(200).send({
          ok: false,
          message: "Secret not found.",
        });
      // Accessible secret found
      case 1:
        return res.status(200).send({
          ok: true,
          secret: wasmInstance.UTF8ToString(secretPtr),
        });
    }
  } catch (error) {
    console.error("[!] Error while handling request:", error);
    res.status(500).send({
      ok: false,
      message: "Internal server error.",
    });
  } finally {
    // free the allocated memory for the status pointer
    if (statusPtr) wasmInstance._free(statusPtr);
    if (queryPtr) wasmInstance._free(queryPtr);
    if (secretPtr) wasmInstance._free(secretPtr);
  }
});

// start the application
startApp();

// stop the application gracefully on SIGINT
process.on("SIGINT", () => {
  process.exit(0);
})

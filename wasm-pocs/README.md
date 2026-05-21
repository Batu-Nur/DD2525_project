# WASM Vulnerabilities PoC

This repository contains some proof-of-concept code on how C binary vulnerabilities can lead to web security vulnerabilities.

The PoCs are about the following binary vulnerabilities:

* Stack-Based Buffer Overflow
* Uncontrolled Format String
* Use After Free
* Integer Overflow

And the following web security vulnerabilities:

* SQL Injections
* Server Side Template Injection
* Cross-Site Leak

## Usage

The only requirement to run the proof-of-concepts is `docker`.

You can clone this repository and run a PoC with the `run.sh` script. The script takes the following parameters:

* `` (nothing), builds the wasm and runs the application at port `3000`
* `-c`, clears the built wasm and cache
* `-b`, builds the wasm files

You can then execute the relative `python` script to exploit the binary vulnerability and obtain a proof-of-concept. The only exception is for XS-Leaks: the python script exploits a different scenario where the leaked secret is from the same account that added it. Instead, you can find a step-by-step guide to exploit this in a normal environment under `exploit/steps.txt`.

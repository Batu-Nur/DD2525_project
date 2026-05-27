import subprocess
import json
import os
import sys

def run_wasmati_queries(wasmati_path, wasm_file):
    result = subprocess.run([wasmati_path, "--native", wasm_file], capture_output=True, text=True)
    return result.stdout, result.stderr

def parse_wasmati_output(output):
    try:
        return json.loads(output)
    except json.JSONDecodeError:
        print("Failed to parse Wasmati output as JSON.")
        return None
    
def evaluate_wasmati(wasmati_path, wasm_file):
    output, error = run_wasmati_queries(wasmati_path, wasm_file)

    if error: 
        print(f"Error running Wasmati: {error}")
        return None
    
    results = parse_wasmati_output(output)
    if results is not None:
        print("Wasmati Query Results:")
        for query, findings in results.items():
            print(f"{query}: {len(findings)} findings")
           
    else:
        print("No results to display.")
    return results

def evaluate_all_wasm_files(wasmati_path, directory):
    yielded_results = {}
    for filename in os.listdir(directory):
        if filename.endswith(".wasm"):
            print(f"Evaluating {filename}...")
            results = evaluate_wasmati(wasmati_path, os.path.join(directory, filename))
            yielded_results[filename] = results
    return yielded_results

if __name__ == "__main__":
    results = evaluate_all_wasm_files("/Users/jonasjostan/Documents/language_security/project/wasmati/bin/wasmati", "/Users/jonasjostan/Documents/language_security/project/wasmati/tests/c")
    print(json.dumps(results, indent=2))
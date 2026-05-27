import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed, wait, FIRST_COMPLETED
from itertools import islice
import json
import os
import random
import subprocess


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
    return parse_wasmati_output(output)


def evaluate_one_file(wasmati_path, file_path):
    print(f"Evaluating {file_path}")
    results = evaluate_wasmati(wasmati_path, file_path)
    return file_path, results


def load_done_files_jsonl(path):
    done = set()
    if not os.path.exists(path):
        return done
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                row = json.loads(line)
            except json.JSONDecodeError:
                continue
            file_path = row.get("file") if isinstance(row, dict) else None
            if isinstance(file_path, str):
                done.add(file_path)
    return done


def append_result_jsonl(path, file_path, result):
    row = {"file": file_path, "results": result}
    with open(path, "a", encoding="utf-8") as f:
        f.write(json.dumps(row, ensure_ascii=False) + "\n")
        f.flush()


def collect_wasm_files(directory, recursive=True):
    wasm_files = []
    if recursive:
        for root, _, filenames in os.walk(directory):
            for filename in filenames:
                if filename.endswith(".wasm"):
                    wasm_files.append(os.path.join(root, filename))
    else:
        for filename in os.listdir(directory):
            p = os.path.join(directory, filename)
            if os.path.isfile(p) and filename.endswith(".wasm"):
                wasm_files.append(p)
    return sorted(wasm_files)


def filter_files_by_size_kb(wasm_files, max_size_kb):
    if not max_size_kb or max_size_kb <= 0:
        return wasm_files, []

    kept = []
    skipped = []
    max_bytes = int(max_size_kb * 1024)

    for p in wasm_files:
        try:
            size = os.path.getsize(p)
        except OSError:
            skipped.append((p, None))
            continue

        if size <= max_bytes:
            kept.append(p)
        else:
            skipped.append((p, size))

    return kept, skipped

def bounded(executor, func, items, max_pending):
    it = iter(items)
    pending = {executor.submit(func, item) for item in islice(it, max_pending)}
    while pending:
        done, pending = wait(pending, return_when=FIRST_COMPLETED)
        for fut in done:
            yield fut.result()
        for item in islice(it, len(done)):
            pending.add(executor.submit(func, item))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Evaluate wasm files and append results to JSONL")
    parser.add_argument("--wasmati", default="/Users/jonasjostan/Documents/language_security/project/wasmati/bin/wasmati")
    parser.add_argument("--directory", default="/Users/jonasjostan/Documents/language_security/project/wasmati/tests/c")
    parser.add_argument("--recursive", action="store_true", help="Recursively scan for .wasm files")
    parser.add_argument("--limit", type=int, default=0, help="Analyze only first N files (0 = all)")
    parser.add_argument("--sample", action="store_true", help="Randomly sample files before applying --limit")
    parser.add_argument("--seed", type=int, default=42, help="Seed for --sample")
    parser.add_argument("--workers", type=int, default=1, help="Number of parallel workers (1 = sequential)")
    parser.add_argument("--output-jsonl", default="results.jsonl", help="Path to output JSONL file")
    parser.add_argument("--overwrite-existing", action="store_true", help="Recompute and append even if file already exists in JSONL")
    parser.add_argument("--max-size-kb", type=int, default=400, help="Skip files larger than this size in KB (0 = disable)")
    parser.add_argument("--batch", type=int, default=-1, help="selects the batch size for the evaluation. If -1, no batching is used and each file is evaluated separately.")
    args = parser.parse_args()

    all_files = collect_wasm_files(args.directory, recursive=args.recursive)
    all_files, skipped_large = filter_files_by_size_kb(all_files, args.max_size_kb)

    if skipped_large:
        print(f"Skipped large files: {len(skipped_large)} (>{args.max_size_kb} KB)")

    if args.sample and all_files:
        rng = random.Random(args.seed)
        rng.shuffle(all_files)

    if args.limit and args.limit > 0:
        all_files = all_files[:args.limit]

    all_files = [os.path.abspath(p) for p in all_files]

    done = load_done_files_jsonl(args.output_jsonl)
    if args.overwrite_existing:
        todo_files = all_files
    else:
        todo_files = [p for p in all_files if p not in done]
    del done
    
    print(f"Total files selected: {len(all_files)}")
    print(f"Already analyzed: {len(all_files) - len(todo_files)}")
    print(f"To analyze now: {len(todo_files)}")

    if args.workers <= 1:
        for file_path in todo_files:
            fp, res = evaluate_one_file(args.wasmati, file_path)
            
            append_result_jsonl(args.output_jsonl, fp, res)
    else:
        with ThreadPoolExecutor(max_workers=args.workers) as executor:
            for file_path, res in bounded(executor, lambda p: evaluate_one_file(args.wasmati, p), todo_files, max_pending=args.workers * 2):
                append_result_jsonl(args.output_jsonl, file_path, res)
                del res
    print(f"Saved results to {os.path.abspath(args.output_jsonl)}")

#!/usr/bin/env python3
import argparse
import json
from pathlib import Path


def normalize_language_name(name: str) -> str:
    # Simple normalization for common labels in producers.language
    return name.replace('_plus_plus_', '++').replace('_', ' ')


def infer_language_from_compiler(entry: dict) -> str | None:
    producers = entry.get('producers') or {}
    processed_by = producers.get('processed-by') or {}
    if not isinstance(processed_by, dict) or not processed_by:
        return None

    keys = ' '.join(str(k).lower() for k in processed_by.keys())

    if any(x in keys for x in ['rustc']):
        return 'Rust'
    if any(x in keys for x in ['tinygo']):
        return 'Go'
    if any(x in keys for x in ['assemblyscript']):
        return 'AssemblyScript'
    if any(x in keys for x in ['clang', 'gcc', 'g++', 'emscripten']):
        return 'C/C++'
    if any(x in keys for x in ['go']):
        return 'Go'
    return None


def extract_language(entry: dict) -> str | None:
    producers = entry.get('producers') or {}
    language_obj = producers.get('language') or {}
    if not isinstance(language_obj, dict) or not language_obj:
        return infer_language_from_compiler(entry)

    langs = [normalize_language_name(k) for k in language_obj.keys()]
    langs = sorted(set(langs))
    return ', '.join(langs) if langs else None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Adds the language.'
    )
    parser.add_argument('--input', default='my_results_max25kb.jsonl', help='Input JSONL')
    parser.add_argument('--meta', default='filtered-binaries-metadata/filtered.pretty.json', help='Metadata JSON')
    parser.add_argument('--output', default='my_results_max25kb_with_language.jsonl', help='Output JSONL')
    parser.add_argument('--field', default='language', help='Name of the field to add')
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    input_path = Path(args.input)
    meta_path = Path(args.meta)
    output_path = Path(args.output)

    with meta_path.open('r', encoding='utf-8') as f:
        meta = json.load(f)

    hash_to_lang = {}
    for file_hash, entry in meta.items():
        if not isinstance(entry, dict):
            continue
        hash_to_lang[file_hash] = extract_language(entry)

    total = 0
    matched = 0
    inferred = 0
    missing = 0

    with input_path.open('r', encoding='utf-8') as fin, output_path.open('w', encoding='utf-8') as fout:
        for raw_line in fin:
            line = raw_line.strip()
            if not line:
                continue

            obj = json.loads(line)
            total += 1

            file_path = obj.get('file', '')
            file_name = Path(file_path).name
            file_hash = file_name[:-5] if file_name.endswith('.wasm') else file_name

            language = hash_to_lang.get(file_hash)
            if language is not None:
                obj[args.field] = language
                matched += 1
                meta_entry = meta.get(file_hash)
                if isinstance(meta_entry, dict):
                    language_obj = (meta_entry.get('producers') or {}).get('language') or {}
                    if not language_obj:
                        inferred += 1
            else:
                obj[args.field] = None
                missing += 1

            fout.write(json.dumps(obj, ensure_ascii=False) + '\n')



if __name__ == '__main__':
    main()

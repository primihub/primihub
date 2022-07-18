#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# test_zmq_async.py
# @Author :  ()
# @Link   :
# @Date   : 6/23/2022, 6:41:43 PM

import asyncio
import zmq
import zmq.asyncio
import zmq.asyncio

ctx = zmq.asyncio.Context()
context = zmq.Context()


async def async_process(msg):
    print("exec will sleep 3...")
    await asyncio.sleep(3)
    print(msg)
    return msg


url = "tcp://127.0.0.1:5557"

r_socket = ctx.socket(zmq.PULL)
r_socket.bind("tcp://127.0.0.1:5558")


async def node(n):
    print("node waiting...")
    await asyncio.sleep(n)
    # context = zmq.Context()
    sock = context.socket(zmq.PUSH)
    sock.connect("tcp://127.0.0.1:5557")
    print("push: ", sock)
    sock.send_json(n)

    res = r_socket.recv()
    print("recv:--------- ", await res)

    return n


async def server(node_num=0):
    node_list = []
    server_sock = ctx.socket(zmq.PULL)
    server_sock.bind("tcp://127.0.0.1:5557")
    for i in range(node_num):
        print("server waiting...", i)
        print("pull: ", server_sock)
        msg = await server_sock.recv_multipart()  # waits for msg to be ready
        node_list.append(msg)
    print("server rec: ", node_list)
    reply = await async_process(node_list)
    print("reply: ", reply)

    consumer_sender = ctx.socket(zmq.PUSH)
    consumer_sender.connect("tcp://127.0.0.1:5558")
    for i in range(node_num):
        print("re send to client: ", i)
        print("---------send.")
        print(type(reply), reply, len(reply), str(reply))
        consumer_sender.send_string(str(reply))
    return reply


async def main():
    tasks = [node(1), node(2), node(4), server(3)]
    complete, pending = await asyncio.wait(tasks)
    for i in complete:
        print("result: ", i.result())
    if pending:
        for p in pending:
            print(p)


if __name__ == "__main__":

    loop = asyncio.get_event_loop()
    try:
        loop.run_until_complete(main())
    finally:
        loop.close()

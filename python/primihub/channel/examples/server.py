import sys
from os import path

sys.path.append(path.abspath(path.join(path.dirname(__file__), "../../")))
import asyncio
from channel.consumer import Consumer
from channel.producer import Producer

def prodocer():
    return Producer("127.0.0.1", "5558")

def consumer():
    return Consumer("127.0.0.1", "5557")


async def async_process(msg):
    # mock func
    await asyncio.sleep(3)
    print("Have a cup of coffee: ", msg)
    for d in msg:
        d['data'] += 1
    return msg


async def server(node_count=0):
    producer = producer()
    consumer = consumer()
    node_list = []
    for i in range(node_count):
        print("server watting for node ...", i)
        msg = await consumer.recv()  # waits for msg to be ready
        import json
        node_list.append(json.loads(msg))
    print("server rec: ", node_list)
    reply = await async_process(node_list)
    print("reply: ", reply)

    for i in range(node_count):
        print("send to node: ", i)
        producer.send(reply)
    return reply


async def main():
    tasks = [server(3)]
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

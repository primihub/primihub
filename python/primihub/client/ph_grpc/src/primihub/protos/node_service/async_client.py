"""
 Copyright 2022 PrimiHub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 """
import asyncio
import logging
import sys

import grpc
import service_pb2
import service_pb2_grpc
from primihub.client import tiny_listener
from primihub.client.tiny_listener import Listener, Event

# NodeEventType
from primihub.utils.protobuf_to_dict import protobuf_to_dict

NODE_EVENT_TYPE_NODE_CONTEXT = 0
NODE_EVENT_TYPE_TASK_STATUS = 1
NODE_EVENT_TYPE_TASK_RESULT = 2
listener = Listener()
listener.run()


# from protobuf_to_dict import protobuf_to_dict
class EventHandlers(object):

    @listener.on_event(str(NODE_EVENT_TYPE_NODE_CONTEXT))
    async def handler_node_context(self):
        print("handler_node_context")

    @listener.on_event(str(NODE_EVENT_TYPE_TASK_STATUS))
    async def handler_task_status(self, event: Event):
        print("handler_task_status", event.params, event.data)
        # print(event)

    @listener.on_event(str(NODE_EVENT_TYPE_TASK_RESULT))
    async def handler_task_result(self, event: Event):
        print("handler_task_result", event.params, event.data)


async def _run() -> None:
    async with grpc.aio.insecure_channel("192.168.99.23:6666") as channel:
        print(channel)
        print("192.168.99.23:6666")
        stub = service_pb2_grpc.NodeServiceStub(channel)
        # Read from an async generator
        print("# Read from an async generator")
        async for response in stub.SubscribeNodeEvent(
                service_pb2.ClientContext(
                    client_id="1",
                    # client_ip="127.0.0.1",
                    # client_port=50051
                )):
            print("NodeService client received from async generator: " +
                  str(response.event_type))
            print("NodeService client received from async generator: " + response.event_type)
        print("# Direct read from the stub")
        # Direct read from the stub
        node_event_reply_stream = stub.SubscribeNodeEvent(
            service_pb2.ClientContext(
                client_id="1",
                # client_ip="127.0.0.1",
                # client_port=50051
            ))
        while True:
            response = await node_event_reply_stream.read()
            if response == grpc.aio.EOF:
                break
            print("NodeService client received from direct read: " +
                  str(response.event_type))
            print("NodeService client received from async generator: " + response.event_type)


def callback():
    print("callback...")


# 0.0.0.0:6666
async def run_async() -> None:
    async with grpc.aio.insecure_channel("192.168.99.23:6666") as channel:
        print("192.168.99.23:6666")
        # async with grpc.aio.insecure_channel("localhost:50051") as channel:
        stub = service_pb2_grpc.NodeServiceStub(channel)
        # Read from an async generator
        print("# Read from an async generator")
        async for response in stub.SubscribeNodeEvent(
                service_pb2.ClientContext(
                    client_id="client01",
                    # client_ip="127.0.0.1",
                    # client_port=50051
                )):
            print("NodeService client received from async generator: " +str(response.event_type))
            # listener.fire(str(response.event_type))
            # listener.fire(str(response))
            print(">>>>>>>>><<<<<<<<<<<<<<<<")
            # print(protobuf_to_dict(response))
            response_dict = protobuf_to_dict(response)
            print(type(response_dict), response_dict)
            listener.fire(name=str(response.event_type), data=response_dict)


async def run_direct():
    async with grpc.aio.insecure_channel("localhost:50051") as channel:
        stub = service_pb2_grpc.NodeServiceStub(channel)
        # Direct read from the stub
        node_event_reply_stream = stub.SubscribeNodeEvent(
            service_pb2.ClientContext(
                client_id="client01",
                # client_ip="127.0.0.1",
                # client_port=50051
            ))
        while True:
            response = await node_event_reply_stream.read()
            if response == grpc.aio.EOF:
                break
            print("NodeService client received from direct read: " +
                  str(response.event_type))


async def func():
    while True:
        print("<<<<<<<<<<,")
        await asyncio.sleep(1)


async def listen(app_dir: str, app: str) -> None:
    # sys.path.insert(0, app_dir)
    # try:
    #     listener_run: tiny_listener.Listener = tiny_listener.import_from_string(app)
    # except BaseException as e:
    #     # click.echo(e)
    #     print(e)
    #     sys.exit(1)
    # else:
    #     listener_run.run()
    listener.run()

# async def run():
#     listener.run()


# async def m():
#     listen(".", "async_client:listener")


async def main():
    complete, pending = await asyncio.wait([run_async(), func()])
    for i in complete:
        print("result: ", i.result())
    if pending:
        for p in pending:
            print(p)


if __name__ == "__main__":
    logging.basicConfig()
    # asyncio.run(run())
    # asyncio.run(func())
    loop = asyncio.get_event_loop()
    try:
        loop.run_until_complete(main())
    finally:
        loop.close()

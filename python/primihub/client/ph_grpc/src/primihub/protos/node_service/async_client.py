"""
 Copyright 2022 Primihub

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

import grpc
import service_pb2
import service_pb2_grpc


async def _run() -> None:
    async with grpc.aio.insecure_channel("localhost:50051") as channel:
        stub = service_pb2_grpc.NodeServiceStub(channel)
        # Read from an async generator
        print("# Read from an async generator")
        async for response in stub.SubscribeNodeEvent(
                service_pb2.ClientContext(
                    client_id="client01",
                    client_ip="127.0.0.1",
                    client_port=50051
                )):
            print("NodeService client received from async generator: " +
                  str(response.event_type))
        print("# Direct read from the stub")
        # Direct read from the stub
        node_event_reply_stream = stub.SubscribeNodeEvent(
            service_pb2.ClientContext(
                client_id="client01",
                client_ip="127.0.0.1",
                client_port=50051
            ))
        while True:
            response = await node_event_reply_stream.read()
            if response == grpc.aio.EOF:
                break
            print("NodeService client received from direct read: " +
                  str(response.event_type))


async def run():
    async with grpc.aio.insecure_channel("localhost:50051") as channel:
        stub = service_pb2_grpc.NodeServiceStub(channel)
        # Direct read from the stub
        node_event_reply_stream = stub.SubscribeNodeEvent(
            service_pb2.ClientContext(
                client_id="client01",
                client_ip="127.0.0.1",
                client_port=50051
            ))
        while True:
            response = await node_event_reply_stream.read()
            if response == grpc.aio.EOF:
                break
            print("NodeService client received from direct read: " +
                  str(response.event_type))
        # await streaming()


async def func():
    while True:
        print("<<<<<<<<<<,")
        await asyncio.sleep(2)


async def main():
    complete, pending = await asyncio.wait([run(), func()])
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

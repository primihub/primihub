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
import random

import grpc

from service_pb2 import ClientContext, NodeEventReply
from service_pb2_grpc import NodeServiceServicer, add_NodeServiceServicer_to_server


class NodeService(NodeServiceServicer):
    async def SubscribeNodeEvent(self, request: ClientContext,
                           context: grpc.aio.ServicerContext) -> NodeEventReply:
        logging.info("Serving SubscribeNodeEvent request: %s", request)
        for i in range(10):
            import time
            time.sleep(i)
            event_type = random.randint(0, 2)
            print("event_type: %s" % event_type)
            yield NodeEventReply(
                event_type=event_type,
                task_status={
                    "task_context": {
                        "node_id": "node-%s" % i,
                        "task_id": "task-%s" % i,
                        "job_id": "job-%s" % i
                    },
                    "status": "RUNNING"
                }
            )


async def serve() -> None:
    server = grpc.aio.server()
    add_NodeServiceServicer_to_server(NodeService(), server)
    listen_addr = "[::]:50051"
    server.add_insecure_port(listen_addr)
    logging.info("Starting server on %s", listen_addr)
    await server.start()
    await server.wait_for_termination()


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    asyncio.run(serve())

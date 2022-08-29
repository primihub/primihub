"""
Copyright 2022 Primihub

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     https: //www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

import primihub as ph
from primihub.client import primihub_cli as cli

# client init
# cli.init(config={"node": "127.0.0.1:8050", "cert": ""})
# cli.init(config={"node": "192.168.99.26:8050", "cert": ""})
# cli.init(config={"node": "192.168.99.26:50050", "cert": ""})
cli.init(config={"node": "192.168.99.23:50050", "cert": ""})

# from primihub import context, dataset

# print(ph.context.Context.__dict__)

# ph.dataset.define("guest_dataset")
# ph.dataset.define("label_dataset")


# # ph.dataset.define("test_dataset")

# # define a remote method
# @ph.context.function(role='host', protocol='xgboost', datasets=["label_dataset"], next_peer="*:5555")
# def func1(value=1):
#     print("params: ", str(value))

#     # do something
#     # next_peer = ph.context.Context.nodes_context["host"].next_peer
#     # print("host next peer: ", next_peer)
#     # ip, port = next_peer.split(":")
#     # ios = IOService()
#     # server = Session(ios, ip, port, "server")
#     # channel = server.addChannel()
#     # channel.recv()
#     # channel.send(value)
#     # print(channel.recv())
#     # return value


# # define a remote method
# @ph.context.function(role='guest', protocol='xgboost', datasets=["guest_dataset"], next_peer="localhost:5555")
# def func2(value=2):
#     print("params: ", str(value))
#     # do something
#     # next_peer = ph.context.Context.nodes_context["host"].next_peer
#     # print("guest next peer: ", next_peer)
#     # ip, port = next_peer.split(":")
#     # ios = IOService()
#     # client = Session(ios, ip, port, "client")
#     # channel = client.addChannel()
#     # channel.send(b'guest ready')
#     # pub = channel.recv()
#     # channel.send(b'recved pub')
#     # return value


# # context
# value1 = 1
# value2 = 2
# # cli.async_remote_execute((func1, value1), (func2, value2)) # 1111
# print(ph.context.Context)
# 
print("###########")
cli.start()
print("###########")

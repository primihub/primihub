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
import logging
import pickle
import time

import primihub as ph
from primihub import dataset, context
from primihub.channel.redis_channel import IOService, Session
from primihub.client import primihub_cli as cli


def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT, datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


logger = get_logger("hetero_xgb")
# client init
# cli.init(config={"node": "127.0.0.1:50050", "cert": ""})
# cli.init(config={"node": "192.168.99.23:8050", "cert": ""})
cli.init(config={"node": "192.168.99.26:50050", "cert": ""})

# Number of tree to fit.
num_tree = 1
# Max depth of each tree.
max_depth = 1

dataset.define("guest_dataset")
dataset.define("label_dataset")

ph.context.Context.func_params_map = {
    "xgb_host_logic": ("paillier",),
    "xgb_guest_logic": ("paillier",)
}


@ph.context.function(role='host', protocol='xgboost', datasets=['label_dataset'], port='8000', task_type="regression")
def xgb_host_logic():
    logger.info("start xgb host logic...")

    uuid = ph.context.Context.topic
    ios = IOService()
    server = Session(ios, ip='192.168.99.31', port='15550', session_mode=None)
    channel_name = f"{uuid}:host"
    print(">>>>>", channel_name)
    channel = server.addChannel(channel_name)

    for i in range(10):
        channel.pub(str(i))  # pub
        print(i)
        time.sleep(3)

    print("end.......")


@ph.context.function(role='guest', protocol='xgboost', datasets=['guest_dataset'], port='9000', task_type="regression")
def xgb_guest_logic():
    logger.debug("*********** start xgb guest logic...")

    uuid = ph.context.Context.topic
    ios = IOService()
    server = Session(ios, ip='192.168.99.31', port='15550', session_mode=None)
    channel_name = f"{uuid}:host"
    print(">>>>>", channel_name)
    channel = server.addChannel(channel_name)
    msg = channel.sub()  # sub

    while True:
        item = next(msg)
        print(type(item), item)
        if item["type"] == "message":
            print(item["channel"], 111)
            data = {str(item["channel"], encoding="utf-8"): str(item["data"], encoding="utf-8")}
        elif item["type"] == "subscribe":
            print(item["channel"], 222)
            data = {str(item["channel"], encoding="utf-8"): 'Subscribe successful'}

        print(data)

    print("end.......")


# context
cry_pri = "plaintext"
# cry_pri = "paillier"
cli.async_remote_execute((xgb_host_logic, cry_pri), (xgb_guest_logic, cry_pri))

# cli.start()

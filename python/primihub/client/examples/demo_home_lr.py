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

import primihub as ph
from primihub import context
from primihub.FL.model.logistic_regression.homo_lr import run_homo_lr_host, run_homo_lr_guest, run_homo_lr_arbiter
from primihub.client import primihub_cli as cli

# client init
# cli.init(config={"node": "127.0.0.1:50050", "cert": ""})
# cli.init(config={"node": "192.168.99.23:8050", "cert": ""})
cli.init(config={"node": "192.168.99.26:50050", "cert": ""})


def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT,
                        datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


# arbiter_info, guest_info, host_info, task_type, task_params = load_info()
task_params = {}
logger = get_logger("Homo-LR")


@ph.context.function(role='arbiter', protocol='lr', datasets=['breast_0'], port='9010', task_type="lr-train")
def run_arbiter_party():
    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map
    data_key = list(dataset_map.keys())[0]

    logger.debug(
        "role_nodeid_map {}".format(role_node_map))

    logger.debug(
        "dataset_map {}".format(dataset_map))

    logger.debug(
        "node_addr_map {}".format(node_addr_map))

    run_homo_lr_arbiter(role_node_map, node_addr_map, data_key)

    logger.info("Finish homo-LR arbiter logic.")


@ph.context.function(role='host', protocol='lr', datasets=['breast_1'], port='9020', task_type="lr-train")
def run_host_party():
    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map

    logger.debug(
        "dataset_map {}".format(dataset_map))
    data_key = list(dataset_map.keys())[0]

    logger.debug(
        "role_nodeid_map {}".format(role_node_map))

    logger.debug(
        "node_addr_map {}".format(node_addr_map))
    logger.info("Start homo-LR host logic.")

    run_homo_lr_host(role_node_map, node_addr_map, data_key)

    logger.info("Finish homo-LR host logic.")


@ph.context.function(role='guest', protocol='lr', datasets=['breast_2'], port='9030', task_type="lr-train")
def run_guest_party():
    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map

    logger.debug(
        "dataset_map {}".format(dataset_map))

    data_key = list(dataset_map.keys())[0]
    logger.debug(
        "role_nodeid_map {}".format(role_node_map))

    logger.debug(
        "node_addr_map {}".format(node_addr_map))
    logger.info("Start homo-LR guest logic.")

    run_homo_lr_guest(role_node_map, node_addr_map, datakey=data_key)

    logger.info("Finish homo-LR guest logic.")


cli.async_remote_execute((run_host_party, ), (run_guest_party, ))

# cli.start()

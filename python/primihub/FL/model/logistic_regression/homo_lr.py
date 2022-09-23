import primihub as ph
import logging
from primihub import dataset, context
from primihub.FL.model.logistic_regression.homo_lr_host import run_homo_lr_host
from primihub.FL.model.logistic_regression.homo_lr_guest import run_homo_lr_guest
from primihub.FL.model.logistic_regression.homo_lr_arbiter import run_homo_lr_arbiter
from os import path
import json
import os

dataset.define("guest_dataset")
dataset.define("label_dataset")

# path = path.join(path.dirname(__file__), '../../../tests/data/wisconsin.data')


def load_info():
    # basedir = os.path.abspath(os.path.dirname(__file__))
    # config_f = open(os.path.join(basedir, 'homo_lr_config.json'), 'r')
    config_f = open(
        '/primihub/python/primihub/FL/model/logistic_regression/homo_lr_config.json',
        'r')
    lr_config = json.load(config_f)
    print(lr_config)
    task_type = lr_config['task_type']
    task_params = lr_config['task_params']
    node_info = lr_config['node_info']
    arbiter_info = {}
    guest_info = {}
    host_info = {}
    for tmp_node, tmp_val in node_info.items():

        if tmp_node == 'Arbiter':
            arbiter_info = tmp_val

        elif tmp_node == 'Guest':
            guest_info = tmp_val

        elif tmp_node == 'Host':
            host_info = tmp_val

        print("=========", tmp_val)

    return arbiter_info, guest_info, host_info, task_type, task_params


def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT,
                        datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


arbiter_info, guest_info, host_info, task_type, task_params = load_info()

logger = get_logger(task_type)

# ph.context.Context.role_nodeid_map["host"] = ["nodeX"]
# ph.context.Context.role_nodeid_map["guest"] = ["nodeY"]
# ph.context.Context.role_nodeid_map["arbiter"] = ["nodeZ"]

# # TODO: Remove them, just for debug.
# ph.context.Context.node_addr_map["nodeX"] = "127.0.0.1:8010"
# ph.context.Context.node_addr_map["nodeY"] = "127.0.0.1:8020"
# ph.context.Context.node_addr_map["nodeZ"] = "127.0.0.1:8030"

# ph.context.Context.dataset_map[
#     'homolr-guest'] = '/Users/xusong/githubs/primihub/data/breast-cancer-wisconsin.data'
# ph.context.Context.dataset_map[
#     'homolr-host'] = '/Users/xusong/githubs/primihub/data/breast-cancer-wisconsin.data'
# ph.context.Context.dataset_map[
#     'homolr-arbiter'] = '/Users/xusong/githubs/primihub/data/breast-cancer-wisconsin.data'

# def dump_task_content(dataset_map, node_addr_map, role_nodeid_map, params_map):
#     logger.info(f"Dataset of all node: {dataset_map}")
#     logger.info(f"Node and it's address: {node_addr_map}")
#     logger.info(f"Role and it's node: {role_nodeid_map}")
#     logger.info(f"Params: {params_map}")


# @ph.context.function(
#     role=host_info[0]['role'],
#     protocol=task_type,
#     #  datasets=host_info[0]['dataset'],
#     datasets=['guest_dataset'],
#     port=str(host_info[0]['port']))
@ph.context.function(role='host', protocol='lr', datasets=['test_dataset'], port='8020', task_type="regression")
def run_host_party():
    logger.info("Start homo-LR host logic.")

    run_homo_lr_host(host_info, arbiter_info, task_params)

    logger.info("Finish homo-LR host logic.")


# @ph.context.function(
#     role=guest_info[0]['role'],
#     protocol=task_type,
#     #  datasets=host_info[0]['dataset'],
#     datasets=["guest_dataset"],
#     port=str(guest_info[0]['port']))
@ph.context.function(role='guest', protocol='lr', datasets=['test_party_0'], port='8010', task_type="regression")
def run_guest_party():
    logger.info("Start homo-LR guest logic.")

    run_homo_lr_guest(guest_info, arbiter_info, task_params)

    logger.info("Finish homo-LR guest logic.")


# Don't forget to add dataset name to ph.context.function's dataset parameters,
# although hetero-LR's arbiter don't handle any examples. This limit comes from
# primihub, primihub use dataset name here to resolve which party will act as
# arbiter.
# @ph.context.function(
#     role=arbiter_info[0]['role'],
#     protocol=task_type,
#     #  datasets=arbiter_info[0]['dataset'],
#     datasets=["label_dataset"],
#     port=str(arbiter_info[0]['port']))
@ph.context.function(role='arbiter', protocol='lr', datasets=['guest_dataset'], port='8030', task_type="regression")
def run_arbiter_party():

    run_homo_lr_arbiter(arbiter_info, guest_info, host_info, task_params)

    logger.info("Finish homo-LR arbiter logic.")


# if __name__ == "__main__":

#     pass

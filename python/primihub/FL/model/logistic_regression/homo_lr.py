import primihub as ph
import logging
from primihub import dataset, context
from primihub.FL.model.logistic_regression.homo_lr_host import run_homo_lr_host
from primihub.FL.model.logistic_regression.homo_lr_guest import run_homo_lr_guest
from primihub.FL.model.logistic_regression.homo_lr_arbiter import run_homo_lr_arbiter
from os import path

# path = path.join(path.dirname(__file__), '../../../tests/data/wisconsin.data')

def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT, datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


logger = get_logger("Homo-LR")

# ph.context.Context.role_nodeid_map["host"] = ["nodeX"]
# ph.context.Context.role_nodeid_map["guest"] = ["nodeY"]
# ph.context.Context.role_nodeid_map["arbiter"] = ["nodeZ"]
#
# # TODO: Remove them, just for debug.
# ph.context.Context.node_addr_map["nodeX"] = "127.0.0.1:8010"
# ph.context.Context.node_addr_map["nodeY"] = "127.0.0.1:8020"
# ph.context.Context.node_addr_map["nodeZ"] = "127.0.0.1:8030"

def dump_task_content(dataset_map, node_addr_map, role_nodeid_map, params_map):
    logger.info(f"Dataset of all node: {dataset_map}")
    logger.info(f"Node and it's address: {node_addr_map}")
    logger.info(f"Role and it's node: {role_nodeid_map}")
    logger.info(f"Params: {params_map}")

@ph.context.function(role='host', protocol='homo-LR',
                     datasets=['host'], port='8010')
def run_host_party():
    logger.info("Start homo-LR host logic.")

    dump_task_content(ph.context.Context.dataset_map,
                      ph.context.Context.node_addr_map,
                      ph.context.Context.role_nodeid_map,
                      ph.context.Context.params_map)

    run_homo_lr_host(ph.context.Context.role_nodeid_map,
                       ph.context.Context.node_addr_map,
                       ph.context.Context.dataset_map["host"],
                       ph.context.Context.params_map)

    logger.info("Finish homo-LR host logic.")


@ph.context.function(role='guest', protocol='homo-LR',
                     datasets=['guest'], port='8020')
def run_guest_party():
    logger.info("Start homo-LR guest logic.")

    dump_task_content(ph.context.Context.dataset_map,
                      ph.context.Context.node_addr_map,
                      ph.context.Context.role_nodeid_map,
                      ph.context.Context.params_map)

    run_homo_lr_guest(ph.context.Context.role_nodeid_map,
                      ph.context.Context.node_addr_map,
                      ph.context.Context.dataset_map["guest"],
                      ph.context.Context.params_map)

    logger.info("Finish homo-LR guest logic.")


# Don't forget to add dataset name to ph.context.function's dataset parameters,
# although hetero-LR's arbiter don't handle any examples. This limit comes from
# primihub, primihub use dataset name here to resolve which party will act as
# arbiter.
@ph.context.function(role='arbiter', protocol='homo-LR',
                     datasets=['arbiter'], port='8030')
def run_arbiter_party():
    logger.info("Start homo-LR arbiter logic.")

    dump_task_content(ph.context.Context.dataset_map,
                      ph.context.Context.node_addr_map,
                      ph.context.Context.role_nodeid_map,
                      ph.context.Context.params_map)

    run_homo_lr_arbiter(ph.context.Context.role_nodeid_map,
                          ph.context.Context.node_addr_map,
                          ph.context.Context.dataset_map["arbiter"],
                          ph.context.Context.params_map)

    logger.info("Finish homo-LR arbiter logic.")

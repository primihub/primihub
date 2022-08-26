import functools
import os
import logging
from typing import Callable
from cloudpickle import dumps
from primihub.utils.logger_util import logger

def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT, datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger

logger = get_logger("context")

class NodeContext:
    def __init__(self, role, protocol, dataset_port_map, func=None):

        self.role = role
        self.protocol = protocol
        self.func = func
        self.dataset_port_map = dataset_port_map 
        self.task_type = None 
        print("func type: ", type(func))

        self.dumps_func = None
        if isinstance(func, Callable):
            # pickle dumps func
            self.dumps_func = dumps(func)
        elif type(func) == str:
            self.dumps_func = func

        if self.dumps_func:
            print("dumps func:", self.dumps_func)
            
        self.datasets = []
        for ds_name in self.dataset_port_map.keys():
            self.datasets.append(ds_name)

    def set_task_type(self, task_type):
        self.task_type = task_type

    def get_task_type(self):
        return self.task_type


class TaskContext:
    """ key: role, value:NodeContext """
    nodes_context = dict()
    dataset_service = None
    datasets = []
    # dataset meta information
    dataset_map = dict()
    node_addr_map = dict()
    predict_file_path = "result/xgb_prediction.csv"
    indicator_file_path = "result/xgb_indicator.json"
    model_file_path = "result/host/model"
    host_lookup_file_path = "result/host/lookuptable"
    guest_lookup_file_path = "result/guest/lookuptable"

    func_params_map = dict()

    def __init__(self) -> None:
        self.role_nodeid_map = {}
        self.role_nodeid_map["host"] = []
        self.role_nodeid_map["arbiter"] = []
        self.role_nodeid_map["guest"] = []
        self.params_map = {}

    def get_protocol(self):
        """Get current task support protocol.
           NOTE: Only one protocol is supported in one task now.
            Maybe support mix protocol in one task in the future.
        Returns:
            string: protocol string
        """
        protocol = None
        try:
            protocol = list(self.nodes_context.values())[0].protocol
        except IndexError:
            protocol = None

        return protocol

    def get_roles(self):
        return list(self.nodes_context.keys())

    def get_datasets(self):
        return self.datasets

    def get_func_params_map(self):
        return self.func_params_map

    def get_task_type(self):
        roles = self.get_roles()
        type_list = []
        for role in roles:
            type_list.append(self.nodes_context[role].get_task_type())

        type_set = set(type_list)
        logger.info(type_set)
        if len(type_set) != 1:
            logger.error("Task type in all role must be the same.")
            raise RuntimeError("Task type in all role is not the same.")
        else:
            return type_list[0] 

    @staticmethod
    def mk_output_dir(output_dir):
        print("output_dir: ", output_dir)
        if output_dir:
            if not os.path.exists(output_dir):
                try:
                    os.makedir(output_dir)
                except:
                    os.makedirs(output_dir)
                finally:
                    print(output_dir)

    def get_predict_file_path(self):
        output_dir = os.path.dirname(self.predict_file_path).strip()
        self.mk_output_dir(output_dir)
        print("predict: ", self.predict_file_path)
        return self.predict_file_path

    def get_indicator_file_path(self):
        output_dir = os.path.dirname(self.indicator_file_path).strip()
        self.mk_output_dir(output_dir)
        print("indicator: ", self.indicator_file_path)
        return self.indicator_file_path

    def get_model_file_path(self):
        output_dir = os.path.dirname(self.model_file_path).strip()
        self.mk_output_dir(output_dir)
        print("model: ", self.model_file_path)
        return self.model_file_path

    def get_host_lookup_file_path(self):
        output_dir = os.path.dirname(self.host_lookup_file_path).strip()
        self.mk_output_dir(output_dir)
        print("host lookup table: ", self.host_lookup_file_path)
        return self.host_lookup_file_path

    def get_guest_lookup_file_path(self):
        output_dir = os.path.dirname(self.guest_lookup_file_path).strip()
        self.mk_output_dir(output_dir)
        print("guest lookup table: ", self.guest_lookup_file_path)
        return self.guest_lookup_file_path
    
    def get_role_node_map(self):
        return self.role_nodeid_map

    def get_node_addr_map(self):
        return self.node_addr_map

    def clean_content(self):
        self.role_nodeid_map.clear()
        self.role_nodeid_map["host"] = []
        self.role_nodeid_map["guest"] = []
        self.role_nodeid_map["arbiter"] = []

        self.node_addr_map.clear()
        self.params_map.clear()


    def get_dataset_service(self, role):
        node_context = self.nodes_context.get(role, None)
        if node_context is None:
            return None
        return node_context.dataset_service_shared_ptr
        
Context = TaskContext()



def set_node_context(role, protocol, datasets):
    dataset_port_map = {}
    for dataset in datasets:
        dataset_port_map[dataset] = "0"
    Context.nodes_context[role] = NodeContext(role, protocol, dataset_port_map)  # noqa

    # TODO set dataset map, key dataset name, value dataset meta information


def set_task_context_func_params(func_name, func_params):
    Context.params_map[func_name] = func_params


def set_task_context_dataset_map(k, v):
    Context.dataset_map[k] = v


def set_task_context_predict_file(f):
    Context.predict_file_path = f


def set_task_context_indicator_file(f):
    Context.indicator_file_path = f


def set_task_context_model_file(f):
    Context.model_file_path = f


def set_task_context_host_lookup_file(f):
    Context.host_lookup_file_path = f


def set_task_context_guest_lookup_file(f):
    Context.guest_lookup_file_path = f


def set_task_context_node_addr_map(node_id_with_role, addr):
    nodeid, role = node_id_with_role.rsplit("_")
    Context.node_addr_map[nodeid] = addr
    Context.role_nodeid_map[role].append(nodeid)
    logger.info("Insert node_id '{}' and it's addr '{}' into task context.".format(nodeid, addr))
    logger.info("Insert role '{}' and nodeid '{}' into task context.".format(role, nodeid))


def set_task_context_params_map(key, value):
    Context.params_map[key] = value
    logger.info("Insert '{}:{}' into task context.".format(key, value))

# For test
def set_text(role, protocol, datasets, dumps_func):
    logger.info("========", role, protocol, datasets, dumps_func)


# Register dataset decorator
def reg_dataset(func):
    @functools.wraps(func)
    def reg_dataset_decorator(dataset):
        print("Register dataset:", dataset)
        Context.datasets.append(dataset)
        return func(dataset)

    return reg_dataset_decorator


# Register task decorator
def function(protocol, role, datasets, port, task_type="default"):
    def function_decorator(func):
        print("Register task:", func.__name__)

        dataset_port_map = {}
        for dataset in datasets:
            dataset_port_map[dataset] = port

        Context.nodes_context[role] = NodeContext(
            role, protocol, dataset_port_map, func)

        Context.nodes_context[role].set_task_type(task_type)

        print(">>>>> dataset_port_map in {}'s node context is {}.".format(
            role, Context.nodes_context[role].dataset_port_map))
        print(">>>>> dataset in {}'s node context is {}.".format(
            role, Context.nodes_context[role].datasets))
        print(">>>>> role in {}'s node context is {}.".format(
            role, Context.nodes_context[role].role))

        @functools.wraps(func)
        def wapper(*args, **kwargs):
            return func(*args, **kwargs)

        return wapper

    return function_decorator

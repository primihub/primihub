import functools
import os
import uuid
from typing import Callable

import zmq
from cloudpickle import dumps
from primihub.utils.logger_util import logger


class NodeContext:
    def __init__(self, role, protocol, dataset_port_map, func=None):

        self.role = role
        self.protocol = protocol
        self.func = func
        self.dataset_port_map = dataset_port_map
        self.task_type = None
        self.dumps_func = None
        self.topic = None

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

    # def set_topic(self, value):
    #     self.topic = value
    #
    # def get_topic(self):
    #     return self.topic
    #


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

    protocol = None
    _uuid = None
    topic = None

    def __init__(self) -> None:
        self.role_nodeid_map = {}
        self.role_nodeid_map["host"] = []
        self.role_nodeid_map["arbiter"] = []
        self.role_nodeid_map["guest"] = []
        self.params_map = {}

    def uuid(self):
        """Return task context UUID string."""
        if not self._uuid:
            self._uuid = uuid.uuid4()
        return self._uuid

    def get_topic(self):
        topic = self.topic
        return topic

    def get_protocol(self):
        """Get current task support protocol.
           NOTE: Only one protocol is supported in one task now.
            Maybe support mix protocol in one task in the future.
        Returns:
            string: protocol string
        """
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
        logger.info(type_list)
        if len(type_set) != 1:
            logger.error("Task type in all role must be the same.")
            # raise RuntimeError("Task type in all role is not the same.")
        else:
            return type_list[0]

    @staticmethod
    def mk_output_dir(output_dir):
        print("output_dir: ", output_dir)
        if output_dir:
            if not os.path.exists(output_dir):
                try:
                    os.makedirs(output_dir)
                except:
                    os.mkdir(output_dir)
                finally:
                    print(output_dir)

    def get_predict_file_path(self):
        file_path = self.params_map.get("predictFileName", None)
        if file_path:
            self.predict_file_path = file_path

        output_dir = os.path.dirname(self.predict_file_path).strip()
        self.mk_output_dir(output_dir)
        logger.info("predict: {}".format(self.predict_file_path))
        return self.predict_file_path

    def get_indicator_file_path(self):
        file_path = self.params_map.get("indicatorFileName", None)
        if file_path:
            self.indicator_file_path = file_path

        output_dir = os.path.dirname(self.indicator_file_path).strip()
        self.mk_output_dir(output_dir)
        logger.info("indicator: {}".format(self.indicator_file_path))
        return self.indicator_file_path

    def get_model_file_path(self):
        file_path = self.params_map.get("modelFileName", None)
        if file_path:
            self.model_file_path = file_path

        output_dir = os.path.dirname(self.model_file_path).strip()
        self.mk_output_dir(output_dir)
        logger.info("model: {}".format(self.model_file_path))
        return self.model_file_path

    def get_host_lookup_file_path(self):
        file_path = self.params_map.get("hostLookupTable", None)
        if file_path:
            self.host_lookup_file_path = file_path

        output_dir = os.path.dirname(self.host_lookup_file_path).strip()
        self.mk_output_dir(output_dir)
        logger.info("host lookup table: {}".format(self.host_lookup_file_path))
        return self.host_lookup_file_path

    def get_guest_lookup_file_path(self):
        file_path = self.params_map.get("guestLookupTable", None)
        if file_path:
            self.guest_lookup_file_path = file_path

        output_dir = os.path.dirname(self.guest_lookup_file_path).strip()
        self.mk_output_dir(output_dir)
        logger.info("guest lookup table: {}".format(
            self.guest_lookup_file_path))
        return self.guest_lookup_file_path

    def get_role_node_map(self):
        return self.role_nodeid_map

    def get_node_addr_map(self):
        return self.node_addr_map

    def clean_content(self):
        self.role_nodeid_map.clear()
        self.nodes_context.clear()
        self.dataset_map.clear()
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


# def set_task_uuid():
#     return Context.uuid()


def set_task_context_dataset_map(k, v):
    Context.dataset_map[k] = v


def set_node_context(role, protocol, topic, datasets):
    dataset_port_map = {}
    for dataset in datasets:
        dataset_port_map[dataset] = "0"
    Context.nodes_context[role] = NodeContext(role, protocol, dataset_port_map)  # noqa
    Context.topic = topic


def set_task_context_node_addr_map(node_id_with_role, addr):
    nodeid, role = node_id_with_role.rsplit("_")
    Context.node_addr_map[nodeid] = addr
    Context.role_nodeid_map[role].append(nodeid)
    logger.info(
        "Insert node_id '{}' and it's addr '{}' into task context.".format(nodeid, addr))
    logger.info(
        "Insert role '{}' and nodeid '{}' into task context.".format(role, nodeid))


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

        logger.debug(">>>>> uuid in {}'s node context is {}.".format(
            role, Context.uuid()))
        logger.debug(">>>>> dataset_port_map in {}'s node context is {}.".format(
            role, Context.nodes_context[role].dataset_port_map))
        logger.debug(">>>>> dataset in {}'s node context is {}.".format(
            role, Context.nodes_context[role].datasets))
        logger.debug(">>>>> role in {}'s node context is {}.".format(
            role, Context.nodes_context[role].role))

        @functools.wraps(func)
        def wapper(*args, **kwargs):
            return func(*args, **kwargs)

        return wapper

    return function_decorator

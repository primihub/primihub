import primihub as ph
from primihub import dataset, context
from phe import paillier
from sklearn import metrics
from primihub.primitive.opt_paillier_c2py_warpper import *
from primihub.FL.model.evaluation.evaluation import Regression_eva
from primihub.FL.model.evaluation.evaluation import Classification_eva
import pandas as pd
import numpy as np
import logging
import pickle

from primihub.primitive.opt_paillier_c2py_warpper import *
import time
import pandas as pd
import numpy as np
import copy
import logging
import time
from concurrent.futures import ThreadPoolExecutor
from primihub.channel.zmq_channel import IOService, Session
import functools
import ray
from ray.util import ActorPool
from line_profiler import LineProfiler

LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
logging.basicConfig(level=logging.DEBUG, format=LOG_FORMAT, datefmt=DATE_FORMAT)
logger = logging.getLogger("proxy")


class ClientChannelProxy:

    def __init__(self, host, port, dest_role="NotSetYet"):
        self.ios_ = IOService()
        self.sess_ = Session(self.ios_, host, port, "client")
        self.chann_ = self.sess_.addChannel()
        self.executor_ = ThreadPoolExecutor()
        self.host = host
        self.port = port
        self.dest_role = dest_role

    # Send val and it's tag to server side, server
    # has cached val when the method return.
    def Remote(self, val, tag):
        msg = {"v": pickle.dumps(val), "tag": tag}
        self.chann_.send(msg)
        _ = self.chann_.recv()
        logger.debug("Send value with tag '{}' to {} finish".format(
            tag, self.dest_role))

    # Send val and it's tag to server side, client begin the send action
    # in a thread when the the method reutrn but not ensure that server
    # has cached this val. Use 'fut.result()' to wait for server to cache it,
    # this makes send value and other action running in the same time.
    def RemoteAsync(self, val, tag):

        def send_fn(channel, msg):
            channel.send(msg)
            _ = channel.recv()

        msg = {"v": val, "tag": tag}
        fut = self.executor_.submit(send_fn, self.chann_, msg)

        return fut


class ServerChannelProxy:

    def __init__(self, port):
        self.ios_ = IOService()
        self.sess_ = Session(self.ios_, "*", port, "server")
        self.chann_ = self.sess_.addChannel()
        self.executor_ = ThreadPoolExecutor()
        self.recv_cache_ = {}
        self.stop_signal_ = False
        self.recv_loop_fut_ = None

    # Start a recv thread to receive value and cache it.
    def StartRecvLoop(self):

        def recv_loop():
            logger.info("Start recv loop.")
            while (not self.stop_signal_):
                try:
                    msg = self.chann_.recv(block=False)
                except Exception as e:
                    logger.error(e)
                    break

                if msg is None:
                    continue

                key = msg["tag"]
                value = msg["v"]
                if self.recv_cache_.get(key, None) is not None:
                    logger.warn(
                        "Hash entry for tag '{}' is not empty, replace old value"
                        .format(key))
                    del self.recv_cache_[key]

                logger.debug("Recv msg with tag '{}'.".format(key))
                self.recv_cache_[key] = value
                self.chann_.send("ok")
            logger.info("Recv loop stops.")

        self.recv_loop_fut_ = self.executor_.submit(recv_loop)

    # Stop recv thread.
    def StopRecvLoop(self):
        self.stop_signal_ = True
        self.recv_loop_fut_.result()
        logger.info("Recv loop already exit, clean cached value.")
        key_list = list(self.recv_cache_.keys())
        for key in key_list:
            del self.recv_cache_[key]
            logger.warn(
                "Remove value with tag '{}', not used until now.".format(key))
        # del self.recv_cache_
        logger.info("Release system resource!")
        self.chann_.socket.close()

    # Get value from cache, and the check will repeat at most 'retries' times,
    # and sleep 0.3s after each check to avoid check all the time.
    def Get(self, tag, max_time=1000, interval=0.1):
        start = time.time()
        while True:
            val = self.recv_cache_.get(tag, None)
            end = time.time()
            if val is not None:
                del self.recv_cache_[tag]
                logger.debug("Get val with tag '{}' finish.".format(tag))
                return pickle.loads(val)

            if (end - start) > max_time:
                logger.warn(
                    "Can't get value for tag '{}', timeout.".format(tag))
                break

            time.sleep(interval)

        return None


class XGBHostInfer:

    def __init__(self, host_tree, host_lookup_table, proxy_server,
                 proxy_client_guest, lr) -> None:
        self.tree = host_tree
        self.lookup = host_lookup_table
        self.proxy_server = proxy_server
        self.proxy_client_guest = proxy_client_guest
        self.learning_rate = lr

    def host_get_tree_node_weight(self, host_test, tree, current_lookup, w):
        if tree is not None:
            k = list(tree.keys())[0]
            role, record_id = k[0], k[1]
            # self.proxy_client_guest.Remote(role, 'role')
            # # print("role, record_id", role, record_id, current_lookup)
            # self.proxy_client_guest.Remote(record_id, 'record_id')

            if role == 'guest':
                ids = self.proxy_server.Get(str(record_id) + '_ids')
                id_left = ids['id_left']
                id_right = ids['id_right']
                host_test_left = host_test.loc[id_left]
                host_test_right = host_test.loc[id_right]
                id_left = id_left.tolist()
                id_right = id_right.tolist()

            else:
                tmp_lookup = current_lookup
                # var, cut = tmp_lookup['feature_id'], tmp_lookup['threshold_value']
                var, cut = tmp_lookup[record_id][0], tmp_lookup[record_id][1]

                host_test_left = host_test.loc[host_test[var] < cut]
                id_left = host_test_left.index.tolist()
                # id_left = host_test_left.index
                host_test_right = host_test.loc[host_test[var] >= cut]
                id_right = host_test_right.index.tolist()
                # id_right = host_test_right.index
                self.proxy_client_guest.Remote(
                    {
                        'id_left': host_test_left.index,
                        'id_right': host_test_right.index
                    },
                    str(record_id) + '_ids')
                # print("==predict host===", host_test.index, id_left, id_right)

            for kk in tree[k].keys():
                if kk[0] == 'left':
                    tree_left = tree[k][kk]
                    w[id_left] = kk[1]
                elif kk[0] == 'right':
                    tree_right = tree[k][kk]
                    w[id_right] = kk[1]
            # print("current w: ", w)
            self.host_get_tree_node_weight(host_test_left, tree_left,
                                           current_lookup, w)
            self.host_get_tree_node_weight(host_test_right, tree_right,
                                           current_lookup, w)

    def host_predict_prob(self, X):
        pred_y = np.array([self.base_score] * len(X))

        for t in range(len(self.lookup)):
            tree = self.tree[t + 1]
            lookup_table = self.lookup[t + 1]
            # y_t = pd.Series([0] * X.shape[0])
            y_t = np.zeros(len(X))
            #self._get_tree_node_w(X, tree, lookup_table, y_t, t)
            self.host_get_tree_node_weight(X, tree, lookup_table, y_t)
            pred_y = pred_y + self.learning_rate * y_t

        return 1 / (1 + np.exp(-pred_y))

    def host_predict(self, X):
        preds = self.predict_prob(X)

        return (preds >= 0.5).astype('int')

    def log_loss(self, actual, predict_prob):

        return metrics.log_loss(actual, predict_prob)


class XGBGuestInfer:

    def __init__(self, guest_tree, guest_lookup_table, proxy_server,
                 proxy_client_host) -> None:
        self.tree = guest_tree
        self.lookup_table = guest_lookup_table
        self.proxy_server = proxy_server
        self.proxy_client_host = proxy_client_host

    def guest_get_tree_ids(self, guest_test, tree, current_lookup):
        if tree is not None:
            k = list(tree.keys())[0]
            role, record_id = k[0], k[1]

            if role == "guest":
                # if record_id is None:
                #     return
                tmp_lookup = current_lookup[record_id]
                var, cut = tmp_lookup[0], tmp_lookup[1]
                guest_test_left = guest_test.loc[guest_test[var] < cut]
                id_left = guest_test_left.index
                guest_test_right = guest_test.loc[guest_test[var] >= cut]
                id_right = guest_test_right.index
                self.proxy_client_host.Remote(
                    {
                        'id_left': id_left,
                        'id_right': id_right
                    },
                    str(record_id) + '_ids')

            else:
                ids = self.proxy_server.Get(str(record_id) + '_ids')
                id_left = ids['id_left']

                guest_test_left = guest_test.loc[id_left]
                id_right = ids['id_right']
                guest_test_right = guest_test.loc[id_right]

            for kk in tree[k].keys():
                if kk[0] == 'left':
                    tree_left = tree[k][kk]
                elif kk[0] == 'right':
                    tree_right = tree[k][kk]

            self.guest_get_tree_ids(guest_test_left, tree_left, current_lookup)
            self.guest_get_tree_ids(guest_test_right, tree_right,
                                    current_lookup)

    def guest_predict(self, X):
        for t in range(len(self.lookup_table)):
            tree = self.tree[t + 1]
            current_lookup = self.lookup_table[t + 1]
            self.guest_get_tree_ids(X, tree, current_lookup)


@ph.context.function(role='host',
                     protocol='xgboost',
                     datasets=['test_hetero_xgb_host'],
                     port='8000',
                     task_type="classification")
def xgb_host_infer():
    logger.info("start xgb host logic...")

    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map
    print("guest info ", role_node_map)

    if len(role_node_map["host"]) != 1:
        logger.error("Current node of host party: {}".format(
            role_node_map["host"]))
        logger.error("In hetero XGB, only dataset of host party has label,"
                     "so host party must have one, make sure it.")
        return

    host_nodes = role_node_map["host"]
    host_port = node_addr_map[host_nodes[0]].split(":")[1]

    guest_nodes = role_node_map["guest"]
    guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")

    proxy_server = ServerChannelProxy(host_port)
    proxy_server.StartRecvLoop()

    host_model_path = ph.context.Context.get_model_file_path() + ".host"
    host_lookup_path = ph.context.Context.get_host_lookup_file_path() + ".host"

    proxy_client_guest = ClientChannelProxy(guest_ip, guest_port, "guest")

    with open(host_model_path, 'rb') as hostModel:
        host_model = pickle.load(hostModel)

    with open(host_lookup_path, 'rb') as hostTable:
        host_table = pickle.load(hostTable)

    xgb_host = XGBHostInfer(host_model['tree_struct'],
                            host_table,
                            proxy_server,
                            proxy_client_guest,
                            lr=host_model['lr'])

    test_host = ph.dataset.read(dataset_key='test_hetero_xgb_host').df_data

    test_y = None
    try:
        test_y = test_host.pop('y')
    except:
        test_y = None

    pred_y = xgb_host.host_predict(test_host)

    if test_y is not None:
        test_acc = metrics.accuracy_score(pred_y, test_y)
        print("test_acc is :", test_acc)

    print("prediction y: ", pred_y)


@ph.context.function(role='guest',
                     protocol='xgboost',
                     datasets=['test_hetero_xgb_guest'],
                     port='8000',
                     task_type="classification")
def xgb_guest_infer():
    logger.info("start xgb guest logic...")

    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map
    print("guest info ", role_node_map)

    if len(role_node_map["host"]) != 1:
        logger.error("Current node of host party: {}".format(
            role_node_map["host"]))
        logger.error("In hetero XGB, only dataset of host party has label,"
                     "so host party must have one, make sure it.")
        return

    guest_nodes = role_node_map["guest"]
    guest_port = node_addr_map[guest_nodes[0]].split(":")[1]
    proxy_server = ServerChannelProxy(guest_port)
    proxy_server.StartRecvLoop()

    host_nodes = role_node_map["host"]
    host_ip, host_port = node_addr_map[host_nodes[0]].split(":")

    proxy_client_host = ClientChannelProxy(host_ip, host_port, "host")

    lookup_file_path = ph.context.Context.get_guest_lookup_file_path(
    ) + ".guest"
    guest_model_path = ph.context.Context.get_model_file_path() + ".guest"

    with open(guest_model_path, 'rb') as guestModel:
        guest_model = pickle.load(guestModel)

    with open(lookup_file_path, 'rb') as guestTable:
        guest_table = pickle.load(guestTable)

    xgb_guest = XGBGuestInfer(guest_model['tree_struct'], guest_table,
                              proxy_server, proxy_client_host)

    test_guest = ph.dataset.read(dataset_key='test_hetero_xgb_guest').df_data
    xgb_guest.guest_predict(test_guest)

    logging.info("End XGBoost guest.")

import primihub as ph
from primihub import dataset, context
from phe import paillier
from sklearn import metrics
from primihub.primitive.opt_paillier_c2py_warpper import *
import pandas as pd
import numpy as np
import logging
import json
import pickle
from scipy.stats import ks_2samp
from sklearn.metrics import roc_auc_score, accuracy_score

from primihub.primitive.opt_paillier_c2py_warpper import *
import time
import pandas as pd
import numpy as np
import copy
import logging
import time
from concurrent.futures import ThreadPoolExecutor
from primihub.channel.zmq_channel import IOService, Session
from primihub.utils.net_worker import GrpcServer
import functools
import ray
from ray.util import ActorPool
from line_profiler import LineProfiler

LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
logging.basicConfig(level=logging.DEBUG, format=LOG_FORMAT, datefmt=DATE_FORMAT)
logger = logging.getLogger("proxy")


def evaluate_ks_and_roc_auc(y_real, y_proba):
    # Unite both visions to be able to filter
    df = pd.DataFrame()
    df['real'] = y_real
    # df['proba'] = y_proba[:, 1]
    df['proba'] = y_proba

    # Recover each class
    class0 = df[df['real'] == 0]
    class1 = df[df['real'] == 1]

    ks = ks_2samp(class0['proba'], class1['proba'])
    roc_auc = roc_auc_score(df['real'], df['proba'])

    print(f"KS: {ks.statistic:.4f} (p-value: {ks.pvalue:.3e})")
    print(f"ROC AUC: {roc_auc:.4f}")
    return ks.statistic, roc_auc


class XGBHostInfer:

    def __init__(self,
                 host_tree,
                 host_lookup_table,
                 proxy_server=None,
                 proxy_client_guest=None,
                 lr=None) -> None:
        self.tree = host_tree
        self.lookup = host_lookup_table
        self.proxy_server = proxy_server
        self.proxy_client_guest = proxy_client_guest
        self.learning_rate = lr
        self.base_score = 0.5

    def host_get_tree_node_weight(self, host_test, tree, current_lookup, w):
        if tree is not None:
            k = list(tree.keys())[0]
            role, record_id = k[0], k[1]
            # self.proxy_client_guest.Remote(role, 'role')
            # # print("role, record_id", role, record_id, current_lookup)
            # self.proxy_client_guest.Remote(record_id, 'record_id')

            if role == 'guest':
                # ids = self.proxy_server.Get(str(record_id) + '_ids')
                ids = self.channel.recv(str(record_id) + '_ids')
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
                # self.proxy_client_guest.Remote(
                #     {
                #         'id_left': host_test_left.index,
                #         'id_right': host_test_right.index
                #     },
                #     str(record_id) + '_ids')

                self.channel.sender(
                    str(record_id) + '_ids', {
                        'id_left': host_test_left.index,
                        'id_right': host_test_right.index
                    })
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
        preds = self.host_predict_prob(X)

        return (preds >= 0.5).astype('int')

    def log_loss(self, actual, predict_prob):

        return metrics.log_loss(actual, predict_prob)


class XGBGuestInfer:

    def __init__(self,
                 guest_tree,
                 guest_lookup_table,
                 proxy_server=None,
                 proxy_client_host=None) -> None:
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
                # self.proxy_client_host.Remote(
                #     {
                #         'id_left': id_left,
                #         'id_right': id_right
                #     },
                #     str(record_id) + '_ids')
                self.channel.sender(
                    str(record_id) + '_ids', {
                        'id_left': id_left,
                        'id_right': id_right
                    })

            else:
                # ids = self.proxy_server.Get(str(record_id) + '_ids')
                ids = self.channel.recv(str(record_id) + '_ids')
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
                     port='8008',
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
    data_key = list(dataset_map.keys())[0]

    host_nodes = role_node_map["host"]
    host_port = node_addr_map[host_nodes[0]].split(":")[1]
    host_ip = node_addr_map[host_nodes[0]].split(":")[0]

    guest_nodes = role_node_map["guest"]
    guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")

    host_model_path = ph.context.Context.get_model_file_path() + ".host"
    host_lookup_path = ph.context.Context.get_host_lookup_file_path() + ".host"

    with open(host_model_path, 'rb') as hostModel:
        host_model = pickle.load(hostModel)

    with open(host_lookup_path, 'rb') as hostTable:
        host_table = pickle.load(hostTable)

    host_channel = GrpcServer(remote_ip=guest_ip,
                              local_ip=host_ip,
                              remote_port=guest_port,
                              local_port=host_port,
                              context=ph.context.Context)

    xgb_host = XGBHostInfer(host_model['tree_struct'],
                            host_table,
                            lr=host_model['lr'])
    xgb_host.channel = host_channel
    test_host = ph.dataset.read(dataset_key=data_key).df_data

    if 'id' in test_host.columns:
        test_host.pop('id')

    test_y = None
    try:
        test_y = test_host.pop('y')
    except:
        test_y = (np.random.random(test_host.shape[0]) > 0.5).astype('int')

    pred_prob = xgb_host.host_predict_prob(test_host)
    acc = accuracy_score((pred_prob >= 0.5).astype('int'), test_y)

    ks, auc = evaluate_ks_and_roc_auc(y_real=test_y, y_proba=pred_prob)
    fpr, tpr, threshold = metrics.roc_curve(test_y, pred_prob)
    test_metrics = {
        "test_acc": acc,
        "test_ks": ks,
        "test_auc": auc,
        "test_fpr": fpr.tolist(),
        "test_tpr": tpr.tolist()
    }

    # pred_y = xgb_host.host_predict(test_host)

    # if test_y is not None:
    #     test_acc = metrics.accuracy_score(pred_y, test_y)
    #     print("test_acc is :", test_acc)

    # print("prediction y: ", pred_y)

    # save preds to file
    predict_file_path = ph.context.Context.get_predict_file_path()
    pred_df = pd.DataFrame({
        'pred_prob': pred_prob,
        "pred_y": (pred_prob >= 0.5).astype('int')
    })
    pred_df.to_csv(predict_file_path, index=False, sep='\t')

    indicator_file_path = ph.context.Context.get_indicator_file_path()

    test_metrics_buff = json.dumps(test_metrics)

    with open(indicator_file_path, 'w') as filePath:
        filePath.write(test_metrics_buff)
        # pickle.dump(test_metrics, filePath)


@ph.context.function(role='guest',
                     protocol='xgboost',
                     datasets=['test_hetero_xgb_guest'],
                     port='9009',
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
    data_key = list(dataset_map.keys())[0]

    guest_nodes = role_node_map["guest"]
    guest_port = node_addr_map[guest_nodes[0]].split(":")[1]
    guest_ip = node_addr_map[guest_nodes[0]].split(":")[0]

    host_nodes = role_node_map["host"]
    host_ip, host_port = node_addr_map[host_nodes[0]].split(":")

    lookup_file_path = ph.context.Context.get_guest_lookup_file_path(
    ) + ".guest"
    guest_model_path = ph.context.Context.get_model_file_path() + ".guest"

    print("guest_model_path: ", guest_model_path)
    guest_channel = GrpcServer(remote_ip=host_ip,
                               remote_port=host_port,
                               local_ip=guest_ip,
                               local_port=guest_port,
                               context=ph.context.Context)

    with open(guest_model_path, 'rb') as guestModel:
        guest_model = pickle.load(guestModel)

    with open(lookup_file_path, 'rb') as guestTable:
        guest_table = pickle.load(guestTable)

    xgb_guest = XGBGuestInfer(guest_model['tree_struct'], guest_table)
    xgb_guest.channel = guest_channel

    test_guest = ph.dataset.read(dataset_key=data_key).df_data

    if 'id' in test_guest.columns:
        test_guest.pop('id')

    xgb_guest.guest_predict(test_guest)

    logging.info("End XGBoost guest.")

import copy
from os import path
from primihub.FL.model.logistic_regression.homo_lr_base import LRModel
import numpy as np
from phe import paillier
from primihub.FL.model.logistic_regression.vfl.evaluation_lr import evaluator
import pandas as pd
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
import logging
from sklearn.datasets import load_iris


def get_logger(name):
    LOG_FORMAT = "[%(asctime)s][%(filename)s:%(lineno)d][%(levelname)s] %(message)s"
    DATE_FORMAT = "%m/%d/%Y %H:%M:%S %p"
    logging.basicConfig(level=logging.DEBUG,
                        format=LOG_FORMAT,
                        datefmt=DATE_FORMAT)
    logger = logging.getLogger(name)
    return logger


logger = get_logger("Homo_LR_Arbiter")


def data_binary(path):
    X1 = pd.read_csv(path, header=None)
    y1 = X1.iloc[:, -1]
    yy = copy.deepcopy(y1)

    for i in range(len(yy.values)):
        if yy[i] == 2:
            yy[i] = 0
        else:
            yy[i] = 1
    X1 = X1.iloc[:, :-1]
    return X1, yy


def iris_data():
    iris = load_iris()
    X = iris.data
    y = iris.target
    return X, y


class Arbiter:
    """
    Tips: Arbiter is a trusted third party !!!
    """

    def __init__(self, proxy_server, proxy_client_host, proxy_client_guest):
        self.need_one_vs_rest = None
        self.public_key = None
        self.private_key = None
        self.need_encrypt = None
        self.epoch = None
        self.weight_host = None
        self.weight_guest = None
        self.theta = None
        self.proxy_server = proxy_server
        self.proxy_client_host = proxy_client_host
        self.proxy_client_guest = proxy_client_guest

    def sigmoid(self, x):
        x = np.array(x, dtype=np.float64)
        y = 1.0 / (1.0 + np.exp(-x))
        return y

    def generate_key(self, length=1024):
        public_key, private_key = paillier.generate_paillier_keypair(
            n_length=length)
        self.public_key = public_key
        self.private_key = private_key

    def broadcast_key(self):
        try:
            self.generate_key()
            logger.info("start send pub")
            self.proxy_client_host.Remote(self.public_key, "pub")
            logger.info("send pub to host OK")
        except Exception as e:
            logger.info("Arbiter broadcast key pair error : %s" % e)

    def predict_prob(self, data):
        if self.need_encrypt:
            global_theta = self.decrypt_vector(self.theta)
            data = np.hstack([np.ones((len(data), 1)), data])
            prob = self.sigmoid(data.dot(global_theta))
            return prob
        else:
            data = np.hstack([np.ones((len(data), 1)), data])
            prob = self.sigmoid(data.dot(self.theta))
            return prob

    def predict_binary(self, prob):
        return np.array(prob > 0.5, dtype='int')

    def predict_one_vs_rest(self, data):
        data = np.hstack([np.ones((len(data), 1)), data])
        if self.need_encrypt == 'YES':
            global_theta = self.decrypt_matrix(self.theta)
            global_theta = np.array(global_theta)
        else:
            global_theta = np.array(self.theta)
        pre = self.sigmoid(data.dot(global_theta.T))
        y_argmax = np.argmax(pre, axis=1)
        return y_argmax

    def predict(self, data, category):
        if category == 2:
            return self.predict_binary(self.predict_prob(data))
        else:
            return self.predict_one_vs_rest(data)

    def evaluation(self, y, y_hat):
        return evaluator.getAccuracy(y, y_hat)

    def model_aggregate(self, host_parm, guest_param, host_data_weight,
                        guest_data_weight):
        param = []
        weight_all = []
        if self.need_encrypt == 'YES':
            if self.need_one_vs_rest == False:
                param.append(self.decrypt_vector(host_parm))
            else:
                param.append(self.decrypt_matrix(host_parm))
            weight_all.append(self.decrypt_vector(host_data_weight)[0])
        else:
            param.append(host_parm)
            weight_all.append(host_data_weight)
        param.append(guest_param)
        weight_all.append(guest_data_weight)
        weight = np.array([weight * 1.0 for weight in weight_all])
        if self.need_one_vs_rest == True:
            agg_param = np.zeros_like(np.array(param))
            for id_c, p in enumerate(param):
                w = weight[id_c] / np.sum(weight, axis=0)
                for id_d, d in enumerate(p):
                    d = np.array(d)
                    agg_param[id_c, id_d] += d * w
            self.theta = np.sum(agg_param, axis=0)
            return list(self.theta)
        else:
            agg_param = np.zeros(len(host_parm))
            for id_c, p in enumerate(param):
                w = weight[id_c] / np.sum(weight, axis=0)
                for id_d, d in enumerate(p):
                    agg_param[id_d] += d * w
            self.theta = agg_param
            return list(self.theta)

    def broadcast_global_model_param(self, host_param, guest_param,
                                     host_data_weight, guest_data_weight):
        self.theta = self.model_aggregate(host_param, guest_param,
                                          host_data_weight, guest_data_weight)
        # send guest plaintext
        self.proxy_client_guest.Remote(self.theta, "global_guest_model_param")
        # send host ciphertext
        if self.need_encrypt == 'YES':
            if self.need_one_vs_rest == False:
                self.theta = self.encrypt_vector(self.theta)
            else:
                self.theta = self.encrypt_matrix(self.theta)

            print("=======global model======", self.theta)
            self.proxy_client_host.Remote(self.theta, "global_host_model_param")
        else:
            self.proxy_client_host.Remote(self.theta, "global_host_model_param")

    def decrypt_vector(self, x):
        return [self.private_key.decrypt(i) for i in x]

    def decrypt_matrix(self, x):
        ret = []
        for r in x:
            ret.append(self.decrypt_vector(r))
        return ret

    def encrypt_vector(self, x):
        return [self.public_key.encrypt(i) for i in x]

    def encrypt_matrix(self, x):
        ret = []
        for r in x:
            ret.append(self.encrypt_vector(r))
        return ret


def run_homo_lr_arbiter(arbiter_info, guest_info, host_info, task_params={}):
    # host_nodes = role_node_map["host"]
    # guest_nodes = role_node_map["guest"]
    # arbiter_nodes = role_node_map["arbiter"]

    if len(host_info) != 1:
        logger.error("Hetero LR only support one host party, but current "
                     "task have {} host party.".format(len(host_info)))
        return

    if len(guest_info) != 1:
        logger.error("Hetero LR only support one guest party, but current "
                     "task have {} guest party.".format(len(guest_info)))
        return

    if len(arbiter_info) != 1:
        logger.error("Hetero LR only support one arbiter party, but current "
                     "task have {} arbiter party.".format(len(arbiter_info)))
        return

    host_info = host_info[0]
    guest_info = guest_info[0]
    arbiter_info = arbiter_info[0]
    # arbiter_port = node_addr_map[arbiter_nodes[0]].split(":")[1]
    arbiter_port = arbiter_info['port']
    proxy_server = ServerChannelProxy(arbiter_port)
    proxy_server.StartRecvLoop()
    logger.debug(
        "Create server proxy for arbiter, port {}.".format(arbiter_port))

    # host_ip, host_port = node_addr_map[host_nodes[0]].split(":")
    host_ip, host_port = host_info['ip'], host_info['port']
    proxy_client_host = ClientChannelProxy(host_ip, host_port, "host")
    logger.debug("Create client proxy to host,"
                 " ip {}, port {}.".format(host_ip, host_port))

    # guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")
    guest_ip, guest_port = guest_info['ip'], guest_info['port']
    proxy_client_guest = ClientChannelProxy(guest_ip, guest_port, "guest")
    logger.debug("Create client proxy to guest,"
                 " ip {}, port {}.".format(guest_ip, guest_port))

    config = {
        'epochs': 1,
        'batch_size': 100,
        'need_one_vs_rest': False,
        'category': 2
    }
    client_arbiter = Arbiter(proxy_server, proxy_client_host,
                             proxy_client_guest)
    client_arbiter.need_one_vs_rest = config['need_one_vs_rest']
    need_encrypt = proxy_server.Get("need_encrypt")

    if need_encrypt == 'YES':
        client_arbiter.need_encrypt = 'YES'
        client_arbiter.broadcast_key()

    batch_num = proxy_server.Get("batch_num")
    host_data_weight = proxy_server.Get("host_data_weight")
    guest_data_weight = proxy_server.Get("guest_data_weight")

    for i in range(config['epochs']):
        logger.info("##### epoch %s ##### " % i)
        for j in range(batch_num):
            logger.info("-----epoch={}, batch={}-----".format(i, j))
            host_param = proxy_server.Get("host_param")
            guest_param = proxy_server.Get("guest_param")
            client_arbiter.broadcast_global_model_param(host_param, guest_param,
                                                        host_data_weight,
                                                        guest_data_weight)
            logger.info("batch={} done".format(j))
        logger.info("epoch={} done".format(i))

    logger.info("####### start predict ######")
    # X, y = iris_data()
    # X, y = data_binary(dataset_filepath)
    data = pd.read_csv(guest_info['dataset'], header=0)
    label = data.pop('y').values
    x = data.copy().values
    # X = LRModel.normalization(X)
    pre = client_arbiter.predict(x, config['category'])
    logger.info('Classification result is:')
    # acc = np.mean(y == pre)
    acc = client_arbiter.evaluation(label, pre)
    logger.info('acc is: %s' % acc)
    logger.info("All process done.")
    proxy_server.StopRecvLoop()
